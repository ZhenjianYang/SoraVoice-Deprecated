#include "Init.h"

#include <Utils/Log.h>
#include <Utils/INI.h>
#include <Utils/ApiPack.h>
#include <RC/RC.h>
#include <RC/RC_SoraRC.h>
#include <asm/asm.h>
#include <SVData.h>
#include <Message.h>
#include <SoraVoice.h>

#include <Windows.h>
#include <dsound.h>

#include <memory>
#include <sstream>
#include <cstring>

constexpr char LOG_FILE_PATH[] = "voice/SoraVoice.log";

static bool LoadRC();
static bool SearchGame();
static bool InitSV();
static bool ApplyMemoryPatch();
static bool DoStart();
static bool DoEnd();

static HMODULE moduleHandle;
static HMODULE dsd_dll;
static HMODULE d3dx_dll;

int StartSoraVoice(void* mh)
{
	::moduleHandle = (HMODULE)mh;

	if(LoadRC() && SearchGame() && InitSV() && ApplyMemoryPatch()){
		DoStart();
		return 1;
	} else {
		DoEnd();
		return 0;
	}
}

int EndSoraVoice()
{
	DoEnd();

	return 1;
}

using namespace std;

using byte = unsigned char;

#define GET_INT(ptr) *(int*)(ptr)
#define GET_SHORT(ptr) *(short*)(ptr)

#define PUT(var, ptr) std::memcpy(ptr, &var, sizeof(var))
#define PUT_ARRAY(arr, ptr) std::memcpy(ptr, arr, sizeof(arr))

constexpr int min_legal_to = 0x100000;

constexpr byte opjmp = 0xE9;
constexpr byte opcall = 0xE8;
constexpr byte opnop = 0x90;

const SVData::Scode scode_sora = { 0x54, 0x5B, 0x5C, 0x5D, 0x5E };
const SVData::Scode scode_za = { 0x55, 0x5C, 0x5D, 0x5E, 0x5F };

constexpr const char * const rc_SoraData[] = {
	"voice/SoraDataEx.ini",
	"voice/SoraData.ini",
};
constexpr char rc_ao_vlist[] = "voice/ao_rnd_vlst.txt";

const string str_jcs = "jcs_";
const string str_from = "_from";
const string str_to = "_to";

const string str_FeatureAddr = "FeatureAddr";
const string str_FeatureValue = "FeatureValue";

const string str_Comment = "Comment";
const string str_Game = "Game";

constexpr const char* addr_list[] = {
	"p_d3dd",
	"p_did",
	"p_Hwnd",
	"p_pDS",
	"p_global",
	"p_keys",
	"p_mute",

	"addr_ppscn",
	"addr_iscn",
};
constexpr int num_addr = sizeof(addr_list) / sizeof(*addr_list);

struct AsmCode {
	const char* name;
	unsigned addr;
	int flags;

	static constexpr int NotForce = 1;
	static constexpr int Jmp = 2;
};
const AsmCode asm_codes[] = {
	{ "text",	(unsigned)ASM::text,	0 },
	{ "dududu",	(unsigned)ASM::dududu,	0 },
	{ "dlgse",	(unsigned)ASM::dlgse,	0 },
	{ "aup",	(unsigned)ASM::aup,		AsmCode::Jmp },
	{ "scode",	(unsigned)ASM::scode,	AsmCode::Jmp },
	{ "ldscn",	(unsigned)ASM::ldscn,	AsmCode::NotForce },
	{ "ldscnB",	(unsigned)ASM::ldscnB,	AsmCode::NotForce },
	{ "ldscnB2",(unsigned)ASM::ldscnB,	AsmCode::NotForce },
};
constexpr int num_asm_codes = sizeof(asm_codes) / sizeof(*asm_codes);

static char buff_ao_vlist[1024];

bool LoadRC()
{
	RC::SetModuleHandle(moduleHandle);
	RC::RcItem rcTable[] = RC_TABLE;
	RC::SetRcTable(rcTable);
	return true;
}

static unsigned GetUIntFromValue(const char* str) {
	if (!str || !str[0]) return 0;

	int rad = 10;
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
		rad = 16;
	}
	char *p;
	return std::strtoul(str, &p, rad);
}

static bool SearchGame(const char* iniName) {
	INI ini; {
		LOG("Open resource: %s", iniName);
		unique_ptr<RC> rc(RC::Get(iniName));
		if (!rc || !rc->First()) {
			LOG("Failed.");
			return false;
		}
		LOG("Resource openned, first = 0x%08X, size = 0x%04X", (unsigned)rc->First(), rc->Size());
		stringstream ss(string(rc->First(), rc->Size()));
		ini.Open(ss);
	}

	if (ini.Num() <= 1) {
		LOG("Bad ini file. %d", ini.Num());
		return false;
	}
	LOG("ini file opened. %d", ini.Num());

	memset(&SV, 0, sizeof(SV));

	int game = 0;
	const INI::Group *group = nullptr;
	SVData::Jcs *jcs = (SVData::Jcs *)&SV.jcs;
	for (int i = 1; i < ini.Num(); i++) {
		auto& tmp_group = ini.GetGroup(i);
		LOG("Check data: %s", tmp_group.Name());

		game = GetUIntFromValue(tmp_group.GetValue(str_Game.c_str()));
		LOG("game = %d", game);
		if (!GAME_IS_VALID(game)) continue;

		bool ok = true;
		for (int j = 0; j < num_asm_codes; j++) {
			string from = str_jcs + asm_codes[j].name + str_from;
			string to = str_jcs + asm_codes[j].name + str_to;

			LOG("name : %s", asm_codes[j].name);

			jcs[j].next = GetUIntFromValue(tmp_group.GetValue(from.c_str()));
			jcs[j].to = GetUIntFromValue(tmp_group.GetValue(to.c_str()));

			LOG("from 0x%08X", jcs[j].next);
			LOG("to 0x%08X", jcs[j].to);

			if (jcs[j].next) {
				unsigned addr_next = jcs[j].next;
				byte* p = (byte*)addr_next;
				constexpr int size = 6;
				if (IsBadReadPtr(p, size)) { ok = false; break; };

				if (jcs[j].to >= min_legal_to) {
					int len_op = p[0] == opjmp || p[0] == opcall ? 5 : 6;
					int jc_len = GET_INT(p + len_op - 4);

					unsigned va_to = addr_next + jc_len + len_op;
					LOG("va_to 0x%08X", va_to);
					if (va_to != jcs[j].to) {
						if (!(asm_codes[j].flags & AsmCode::NotForce)) {
							ok = false; break;
						}
						else {
							jcs[j].next = jcs[j].to = 0;
						}
					}
				}
			}
			else if (!(asm_codes[j].flags & AsmCode::NotForce)) {
				ok = false; break;
			}
		}

		if (ok) {
			unsigned faddr = GetUIntFromValue(tmp_group.GetValue(str_FeatureAddr.c_str()));
			LOG("%s: 0x%08X", str_FeatureAddr.c_str(), faddr);
			if (faddr) {
				unsigned fvalue = GetUIntFromValue(tmp_group.GetValue(str_FeatureValue.c_str()));
				LOG("%s: 0x%08X", str_FeatureValue.c_str(), fvalue);
				unsigned* p = (unsigned*)faddr;
				if (IsBadReadPtr(p, 4)) { ok = false; }
				else {
					LOG("True value: 0x%08X", *p);
					ok = *p == fvalue;
				}
			}
		}

		if (ok) {
			group = &tmp_group;
			break;
		}
	}

	if (!group) {
		LOG("No matched data found in ini");
		return false;
	}
	LOG("Data found, num = %d, name = %s, game = %d", group->Num(), group->Name(), game);

	SV.game = (SVData::Game)game;
	SV.sora = GAME_IS_SORA(SV.game);
	SV.za = GAME_IS_ZA(SV.game);
	SV.tits = GAME_IS_TITS(SV.game);

	memcpy(&SV.scode, GAME_IS_ED6(SV.game) ? &scode_sora: &scode_za, sizeof(SV.scode));

	unsigned * const addrs = (decltype(addrs))&SV.addrs;
	for (int j = 0; j < num_addr; j++) {
		addrs[j] = GetUIntFromValue(group->GetValue(addr_list[j]));
	}

	const char* comment = group->GetValue(str_Comment.c_str());
	if (comment) {
		strncpy(SV.Comment, comment, sizeof(SV.Comment));
	}

	if (SV.game == SVData::AO) {
		unique_ptr<RC> rc_vlist(RC::Get(rc_ao_vlist));
		if (rc_vlist && rc_vlist->First()) {
			int size = (int)rc_vlist->Size();
			const char* pvlst_rst = rc_vlist->First();

			int i, j;
			for (i = 0, j = 0; i < size && j < (int)sizeof(buff_ao_vlist) - 1; i++) {
				if (pvlst_rst[i] != '\r' && pvlst_rst[i] != '\n') {
					buff_ao_vlist[j++] = pvlst_rst[i];
				}
				else if (j > 0 && buff_ao_vlist[j - 1]) {
					buff_ao_vlist[j++] = '\0';
				}
			}
			buff_ao_vlist[j] = '\0';
		}
	}

	return true;
}

bool SearchGame()
{
	for (auto ini_name : rc_SoraData) {
		if (SearchGame(ini_name)) return true;
	}
	return false;
}

using DSD = IDirectSound;
using CallDSCreate = decltype(DirectSoundCreate)*;
constexpr char STR_dsound_dll[] = "dsound.dll";
constexpr char STR_DirectSoundCreate[] = "DirectSoundCreate";

#define BIND(var, ptr) { if (ptr) (var) = decltype(var)(*(ptr)); \
						(ptr) = (decltype(ptr))&(var); }

constexpr char STR_vorbisfile_dll[] = "vorbisfile.dll";
constexpr char STR_libvorbisfile_dll[] = "libvorbisfile.dll";
constexpr char STR_ov_open_callbacks[] = "ov_open_callbacks";
constexpr char STR_ov_info[] = "ov_info";
constexpr char STR_ov_read[] = "ov_read";
constexpr char STR_ov_clear[] = "ov_clear";
constexpr char STR_ov_pcm_total[] = "ov_pcm_total";
constexpr const char* OvDllNames[] = { STR_vorbisfile_dll , STR_libvorbisfile_dll };
constexpr const char* DllDirs[] = { ".\\dll\\", ".\\voice\\dll\\" };

static void* d3dd;
static void* did;
static HWND hWnd;
static DSD* pDS;
static unsigned fake_mute = 1;

constexpr char STR_d3dx_dll_za[] = "d3dx9_42.dll";
constexpr char STR_d3dx_dll_tits[] = "d3dx9_43.dll";
constexpr const char* STR_D3DX9_APIS[][2] = {
	{ "D3DXCreateFontIndirect", "D3DXCreateFontIndirectW" },
	{ "D3DXCreateSprite", "D3DXCreateSprite" }
};

bool InitSV()
{
	LOG("p = 0x%08X", (unsigned)&SV);
	LOG("p->p_d3dd = 0x%08X", (unsigned)SV.addrs.p_d3dd);
	LOG("p->p_did = 0x%08X", (unsigned)SV.addrs.p_did);
	LOG("p->p_Hwnd = 0x%08X", (unsigned)SV.addrs.p_Hwnd);
	LOG("p->p_pDS = 0x%08X", (unsigned)SV.addrs.p_pDS);
	LOG("p->p_mute = 0x%08X", (unsigned)SV.addrs.p_mute);
	LOG("p->p_keys = 0x%08X", (unsigned)SV.addrs.p_keys);
	LOG("p->p_global = 0x%08X", (unsigned)SV.addrs.p_global);
	LOG("p->p_rnd_vlst = 0x%08X", (unsigned)SV.p_rnd_vlst);

	if (GAME_IS_ZA(SV.game) && SV.addrs.p_global) {
		if (SV.addrs.p_d3dd) SV.addrs.p_d3dd = (void**)((char*)*SV.addrs.p_global + (int)SV.addrs.p_d3dd);
		if (SV.addrs.p_did) SV.addrs.p_did = (void**)((char*)*SV.addrs.p_global + (int)SV.addrs.p_did);
		if (SV.addrs.p_Hwnd) SV.addrs.p_Hwnd = (void**)((char*)*SV.addrs.p_global + (int)SV.addrs.p_Hwnd);

		LOG("Adjuested p->p_d3dd = 0x%08X", (unsigned)SV.addrs.p_d3dd);
		LOG("Adjuested p->p_did = 0x%08X", (unsigned)SV.addrs.p_did);
		LOG("Adjuested p->p_Hwnd = 0x%08X", (unsigned)SV.addrs.p_Hwnd);
	}

	BIND(d3dd, SV.addrs.p_d3dd);
	BIND(did, SV.addrs.p_did);
	BIND(hWnd, SV.addrs.p_Hwnd);
	BIND(pDS, SV.addrs.p_pDS);

	LOG("d3dd = 0x%08X", (unsigned)d3dd);
	LOG("did = 0x%08X", (unsigned)did);
	LOG("Hwnd = 0x%08X", (unsigned)hWnd);
	LOG("pDS = 0x%08X", (unsigned)pDS);

	if (!pDS) {
		LOG("pDS is nullptr, now going to creat DirectSoundDevice");
		if (!hWnd) {
			LOG("HWnd is nullptr, cound not DirectSoundDevice");
			return false;
		}
		dsd_dll = NULL;
		dsd_dll = LoadLibraryA(STR_dsound_dll);
		if (dsd_dll) {
			auto pDirectSoundCreate = (CallDSCreate)GetProcAddress(dsd_dll, STR_DirectSoundCreate);
			if (pDirectSoundCreate) {
				ApiPack::AddApi(STR_DirectSoundCreate, (void*)pDirectSoundCreate);
				pDirectSoundCreate(NULL, &pDS, NULL);
				if (pDS) {
					if (DS_OK != pDS->SetCooperativeLevel(hWnd, DSSCL_PRIORITY)) {
						pDS->Release();
						pDS = nullptr;
						LOG("Init DSD failed.");
					}
				}//if (pDS) 
			}//if (pDirectSoundCreate)
		}//if (dsd_dll) 

		LOG("new pDS = 0x%08X", (unsigned)pDS);
	}

	if (!pDS) {
		LOG("pDS is nullptr, failed to init SoraVoice");
		if (dsd_dll) {
			FreeLibrary(dsd_dll);
			dsd_dll = nullptr;
		}
		return false;
	}

	LOG("Now going to get d3dx Apis");

	if (GAME_IS_DX9(SV.game)) {
		d3dx_dll = NULL;
		d3dx_dll = LoadLibrary(GAME_IS_ZA(SV.game) ? STR_d3dx_dll_za : STR_d3dx_dll_tits);
		if (d3dx_dll) {
			for (auto api : STR_D3DX9_APIS) {
				void* ptrApi = (void*)GetProcAddress(d3dx_dll, api[1]);
				if (ptrApi) {
					ApiPack::AddApi(api[0], ptrApi);
					LOG("Api %s (Original:%s) loaded, address = 0x%08X", api[0], api[1], (unsigned)ptrApi);
				}
			}
		}//if (d3dx_dll) 
		else {
			LOG("Load %s failed.", GAME_IS_ZA(SV.game) ? STR_d3dx_dll_za : STR_d3dx_dll_tits);
		}
	}

	{
		LOG("Now going to load vorbisfile.dll ...");

		void* ov_open_callbacks = nullptr;
		void* ov_info = nullptr;
		void* ov_read = nullptr;
		void* ov_clear = nullptr;
		void* ov_pcm_total = nullptr;

		HMODULE ogg_dll = NULL;
		for (auto dir : DllDirs) {
			SetDllDirectoryA(dir);
			for (auto filename : OvDllNames) {
				ogg_dll = LoadLibraryA(filename);
				if (ogg_dll) break;
			}
		}
		SetDllDirectoryA(NULL);
		if (ogg_dll) {
			ov_open_callbacks = (void*)GetProcAddress(ogg_dll, STR_ov_open_callbacks);
			ov_info = (void*)GetProcAddress(ogg_dll, STR_ov_info);
			ov_read = (void*)GetProcAddress(ogg_dll, STR_ov_read);
			ov_clear = (void*)GetProcAddress(ogg_dll, STR_ov_clear);
			ov_pcm_total = (void*)GetProcAddress(ogg_dll, STR_ov_pcm_total);
		}

		LOG("Loaded ov_open_callbacks = 0x%08X", (unsigned)ov_open_callbacks);
		LOG("Loaded ov_info = 0x%08X", (unsigned)ov_info);
		LOG("Loaded ov_read = 0x%08X", (unsigned)ov_read);
		LOG("Loaded ov_clear = 0x%08X", (unsigned)ov_clear);
		LOG("Loaded ov_pcm_total = 0x%08X", (unsigned)ov_pcm_total);

		if (!ov_open_callbacks || !ov_info || !ov_read || !ov_clear || !ov_pcm_total) {
			LOG("Load ogg apis failed.");
			if(ogg_dll) {
				FreeLibrary(ogg_dll);
			}
			return false;
		}
		else {
			ApiPack::AddApi(STR_ov_open_callbacks, ov_open_callbacks);
			ApiPack::AddApi(STR_ov_info, ov_info);
			ApiPack::AddApi(STR_ov_read, ov_read);
			ApiPack::AddApi(STR_ov_clear, ov_clear);
			ApiPack::AddApi(STR_ov_pcm_total, ov_pcm_total);
		}
	}

	Message.LoadMessage();

	return true;
}

bool ApplyMemoryPatch()
{
	SVData::Jcs *jcs = (SVData::Jcs *)&SV.jcs;
	for (int j = 0; j < num_asm_codes; j++) {
		unsigned addr_next = jcs[j].next;
		unsigned addr_type = asm_codes[j].flags;

		byte* from = (byte*)addr_next;
		if (!from) continue;

		int len_op;
		int add_pmute = 0;
		if (jcs[j].to < min_legal_to) {
			len_op = jcs[j].to & 0xFF;
			add_pmute = (jcs[j].to >> 16) & 0xF;
			int to_add = (int8_t)((jcs[j].to >> 8) & 0xFF);

			if (to_add) {
				jcs[j].to = addr_next + to_add;
			}
			else {
				jcs[j].to = 0;
			}

			if (len_op < 5) len_op = 5;
		}
		else {
			len_op = from[0] == opjmp || from[0] == opcall ? 5 : 6;
		}
		jcs[j].next = addr_next + len_op;

		byte* vs_to = (byte*)asm_codes[j].addr;
		int jc_len = vs_to - from - 5;

		char buff[256];
		std::memset(buff, opnop, sizeof(buff));

		const byte op = (addr_type & AsmCode::Jmp) ? opjmp : opcall;
		PUT(op, buff);
		PUT(jc_len, buff + 1);

		LOG("change code at : 0x%08X", (unsigned)from);
		DWORD dwProtect, dwProtect2;
		if (VirtualProtect(from, len_op, PAGE_EXECUTE_READWRITE, &dwProtect)) {
			if (!SV.addrs.p_mute && add_pmute) {
				SV.addrs.p_mute = *(void**)(from + add_pmute);
			}
			std::memcpy(from, buff, len_op);
			VirtualProtect(from, len_op, dwProtect, &dwProtect2);
		}
		else {
			LOG("Change code failed.");
		}
	}

	if (!SV.addrs.p_mute) SV.addrs.p_mute = &fake_mute;
	return true;
}

bool DoStart()
{
	if (GAME_IS_ED6(SV.game)) {
		return SoraVoice::Init();
	}
	else {
		return true;
	}
}

bool DoEnd()
{
	SoraVoice::End();
	if (d3dx_dll) {
		FreeLibrary(d3dx_dll);
		d3dx_dll = nullptr;
	}
	if (dsd_dll) {
		FreeLibrary(dsd_dll);
		dsd_dll = nullptr;
	}
	return true;
}

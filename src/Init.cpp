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
#include <Config.h>

#include <Windows.h>
#include <Psapi.h>

#include <memory>
#include <sstream>
#include <cstring>
#include <algorithm>

constexpr char LOG_FILE_PATH[] = "voice/SoraVoice.log";

static bool LoadRC();
static bool SearchGame();
static bool ApplyMemoryPatch();
static bool DoStart();

static HMODULE moduleHandle;
static MODULEINFO mi_exe;

int StartSoraVoice(void* mh)
{
	LOG("Starting SoraVoice...");

	if (!GetModuleInformation(GetCurrentProcess(), GetModuleHandleA(NULL), &mi_exe, sizeof(mi_exe))) {
		return 0;
	}

	LOG("module info: base = 0x%08X, size = 0x%08X", (unsigned)mi_exe.lpBaseOfDll, (unsigned)mi_exe.SizeOfImage);
	::moduleHandle = (HMODULE)mh;

	LOG("Loading Config...");

	Config.LoadConfig(DFT_CONFIG_FILE);

	LOG("Config loaded");
	LOG("config.Volume = %d", Config.Volume);
	LOG("config.OriginalVoice = %d", Config.OriginalVoice);
	LOG("config.AutoPlay = %d", Config.AutoPlay);
	LOG("config.WaitTimePerChar = %d", Config.WaitTimePerChar);
	LOG("config.WaitTimeDialog = %d", Config.WaitTimeDialog);
	LOG("config.WaitTimeDialogVoice = %d", Config.WaitTimeDialogVoice);
	LOG("config.SkipVoice = %d", Config.SkipVoice);
	LOG("config.DisableDududu = %d", Config.DisableDududu);
	LOG("config.DisableDialogSE = %d", Config.DisableDialogSE);
	LOG("config.ShowInfo = %d", Config.ShowInfo);
	LOG("config.FontName = %s", Config.FontName);
	LOG("config.FontColor = 0x%08X", Config.FontColor);

	LOG("config.EnableKeys = %d", Config.EnableKeys);
	LOG("config.SaveChange = %d", Config.SaveChange);

	if (!LoadRC() || !SearchGame()) return 0;
	if (SERIES_IS_ED6(SV.series) && !InitSVData()) return 0;

	if(ApplyMemoryPatch()) DoStart();

	LOG("SoraVoice Started.");
	return 1;
}

int EndSoraVoice()
{
	SoraVoice::End();
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

constexpr int valid_game_id[] = {
	SORA_FC, SORA_SC, SORA_3RD,
	ZERO, AO,
	SORA_FC + DX8 * 10, SORA_SC + DX8 * 10, SORA_3RD + DX8 * 10,
	SORA_FC + DX9 * 10, SORA_SC + DX9 * 10, SORA_3RD + DX9 * 10,
};

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
	{ "ldscn",	(unsigned)ASM::ldscn,	AsmCode::NotForce | AsmCode::Jmp },
	{ "ldscnB",	(unsigned)ASM::ldscnB,	AsmCode::NotForce },
	{ "ldscnB2",(unsigned)ASM::ldscnB,	AsmCode::NotForce },
	{ "ldquiz",	(unsigned)ASM::ldquiz,	AsmCode::NotForce },
	{ "ldquizB",(unsigned)ASM::ldquizB,	AsmCode::NotForce },
	{ "scnp",	(unsigned)ASM::scnp,	AsmCode::NotForce | AsmCode::Jmp },
	{ "prst",	(unsigned)ASM::prst,	AsmCode::NotForce },
	{ "prst2",	(unsigned)ASM::prst,	AsmCode::NotForce },
	{ "prst3",	(unsigned)ASM::prst,	AsmCode::NotForce },
	{ "prst4",	(unsigned)ASM::prst,	AsmCode::NotForce },
	{ "prst5",	(unsigned)ASM::prst,	AsmCode::NotForce },
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

	std::memset(&SV, 0, sizeof(SV));

	int game = 0;
	const INI::Group *group = nullptr;
	SVData::Jcs *jcs = (SVData::Jcs *)&SV.jcs;
	const unsigned addr_max = (unsigned)mi_exe.lpBaseOfDll + mi_exe.SizeOfImage;
	const unsigned addr_min = (unsigned)mi_exe.lpBaseOfDll;
	for (int i = 1; i < ini.Num(); i++) {
		auto& tmp_group = ini.GetGroup(i);
		LOG("Check data: %s", tmp_group.Name());

		game = GetUIntFromValue(tmp_group.GetValue(str_Game.c_str()));
		LOG("game = %d", game);

		if (std::find(std::begin(valid_game_id), std::end(valid_game_id), game) == std::end(valid_game_id)) continue;

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
				if (jcs[j].next >= addr_max || jcs[j].next < addr_min || jcs[j].to >= addr_max) {
					ok = false;
					break;
				}

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

	SV.game = game % 10;
	SV.dxver = game / 10;
	SV.series = SV.game == ZERO || SV.game == AO ? SERIES_ZEROAO :
						SV.dxver == DXDFT ? SERIES_SORA : SERIES_TITS;
	if(SV.dxver == DXDFT)
		SV.dxver = SV.series == SERIES_ZEROAO ? DX9 : DX8;

	std::memcpy(&SV.scode, SERIES_IS_ED6(SV.series) ? &scode_sora: &scode_za, sizeof(SV.scode));

	unsigned * const addrs = (decltype(addrs))&SV.addrs;
	for (int j = 0; j < num_addr; j++) {
		addrs[j] = GetUIntFromValue(group->GetValue(addr_list[j]));
	}

	const char* comment = group->GetValue(str_Comment.c_str());
	if (comment) {
		strncpy(SV.Comment, comment, sizeof(SV.Comment));
	}

	if (SV.game == AO) {
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
			SV.p_rnd_vlst = buff_ao_vlist;
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
				LOG("Set p_mute = 0x%08X", (unsigned)SV.addrs.p_mute);
				LOG("*p_mute = 0x%08X", SV.addrs.p_mute ? *(unsigned*)SV.addrs.p_mute : 0);
			}
			std::memcpy(from, buff, len_op);
			VirtualProtect(from, len_op, dwProtect, &dwProtect2);
		}
		else {
			LOG("Change code failed.");
		}
	}

	static unsigned fake_mute = 1;
	if (!SV.addrs.p_mute) {
		SV.addrs.p_mute = &fake_mute;
		fake_mute = SV.series == SERIES_ZEROAO ? 1 : 0;

		LOG("Set p_mute to fake_mute : 0x%08X", (unsigned)SV.addrs.p_mute);
		LOG("*p_mute = 0x%08X", SV.addrs.p_mute ? *(unsigned*)SV.addrs.p_mute : 0);
	}
	return true;
}

bool DoStart()
{
	if (SERIES_IS_ED6(SV.series)) {
		return SoraVoice::Init();
	}
	else {
		return true;
	}
}

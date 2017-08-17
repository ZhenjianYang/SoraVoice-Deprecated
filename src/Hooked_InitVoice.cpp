#include "Hooked_InitVoice.h"


#include "ed_voice.h"
#include "INI.h"
#include "InitParam.h"
#include "RC.h"
#include "RC_hk.h"

#include <Windows.h>
#include <dsound.h>
#include <string>
#include <sstream>
#include <memory>

using namespace std;

#define GET_INT(ptr) *(int*)(ptr)
#define GET_SHORT(ptr) *(short*)(ptr)

#define PUT(var, ptr) memcpy(ptr, &var, sizeof(var))
#define PUT_ARRAY(arr, ptr) memcpy(ptr, arr, sizeof(arr))

#ifdef LOG_NOLOG
#define LOG(...)
#else
FILE* _flog;
#define LOG(...) _flog = fopen("voice/hk_log.log", "a+"); fprintf(_flog, __VA_ARGS__), fprintf(_flog, "\n"); fclose(_flog);
#endif // LOG_NOLOG

constexpr int GAME_SORA = 1;
constexpr int GAME_TITS_DX8 = 2;
constexpr int GAME_TITS_DX9 = 3;
constexpr int GAME_ZERO = 11;
constexpr int GAME_AO = 12;
#define IS_VALID_GAME(game) (game == GAME_SORA || game == GAME_TITS_DX8 || game == GAME_TITS_DX9 || game == GAME_ZERO || game == GAME_AO)

constexpr int OFF_VLIST = 0xC00;

constexpr char dll_name_sora[] = "voice/ed_voice.dll";
constexpr char dll_name_sora_dx9[] = "voice/ed_voice_dx9.dll";
constexpr char dll_name_za[] = "voice/za_voice.dll";

constexpr char rc_SoraData[] = "voice/SoraData.ini";
constexpr char rc_SoraDataEx[] = "voice/SoraDataEx.ini";
constexpr char rc_sora_all[] = "voice/sora_all";
constexpr char rc_za_all[] = "voice/za_all";
constexpr char rc_tits_all[] = "voice/tits_all";
constexpr char rc_ao_vlist[] = "voice/ao_rnd_vlst.txt";

constexpr int min_legal_to = 0x100000;

struct SCODE {
	unsigned TEXT;
	unsigned SAY;
	unsigned TALK;
	unsigned MENU;
	unsigned MENUEND;
}
sv.scode.ZA { 0x55, 0x5C, 0x5D, 0x5E, 0x5F},
sv.scode.SORA { 0x54,  0x5B, 0x5C, 0x5D, 0x5E};

constexpr char import_names[][16] = {
	"Init",
	"End",
	"Play",
	"Stop",
	"LoadScn",
	"LoadScns",
};
constexpr int NumImport = sizeof(import_names) / sizeof(*import_names);

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

	"p_mute",
	"p_keys",
	"p_global",
	"addr_ppscn",

	"addr_iscn",
};
constexpr int num_addr = sizeof(addr_list) / sizeof(*addr_list);

constexpr int addr_code = 0x200;
struct AsmCode {
	const char* name;
	unsigned addr;
	bool force;
};
const AsmCode asm_codes[] = {
	{ "text",addr_code + 0x000, true },
	{ "dududu",addr_code + 0x100, true },
	{ "dlgse",addr_code + 0x200, true },
	{ "aup",addr_code + 0x300, true },
	{ "scode",addr_code + 0x400, true },
	{ "ldscn",addr_code + 0x500, false },
	{ "ldscnB",addr_code + 0x600, false },
	{ "ldscnB2",addr_code + 0x600, false },
};
constexpr int num_asm_codes = sizeof(asm_codes) / sizeof(*asm_codes);

using byte = unsigned char;

constexpr byte opjmp = 0xE9;
constexpr byte opcall = 0xE8;
constexpr byte opnop = 0x90;

constexpr byte scode_sora[] = { 0x54, 0x5B, 0x5C, 0x5D, 0x5E };
constexpr byte scode_za[] = { 0x55, 0x5C, 0x5D, 0x5E, 0x5F };

constexpr int Size = 0x1000;

constexpr int addr_bits = 31;
constexpr unsigned addr_mark = (1u << addr_bits) - 1u;

using EDVoice = struct EDVoice {
	HMODULE dll;
	void* ip;
	decltype(::Init)* Init;
	decltype(::End)* End;
};
static EDVoice edvoice;

static bool DoInit(const char* data_name);

bool InitEDVoice(void* hDll) {
	RC::SetHS(hDll);
	RC::RcItem rcTable[] = RC_TABLE;
	RC::SetRcTable(rcTable);

	return DoInit(rc_SoraDataEx) || DoInit(rc_SoraData);
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

static InitParam ip;

__declspec(naked) void rNewShowTextMsg()
{
	__asm {
		push    eax
		push    ebx
		call    ldscn_start
	ldscn_start :
		pop     ebx
		pop     eax
		sub     ebx, 7
		mov     ip.addrs.p_d3dd, eax
		pop     eax

	}
}

bool DoInit(const char* data_name)
{
	INI ini; {
		LOG("Open resource: %s", data_name);
		unique_ptr<RC> rc(RC::Get(data_name));
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
	
	unique_ptr<byte[]> sp = make_unique<byte[]>(Size);
	memset(sp.get(), 0, Size);
	InitParam* tp = (InitParam*)sp.get();

	int game = 0;
	const INI::Group *group = nullptr;
	auto jcs = tp->jcs;
	for (int i = 1; i < ini.Num(); i++) {
		auto& tmp_group = ini.GetGroup(i);
		LOG("Check data: %s", tmp_group.Name());

		game = GetUIntFromValue(tmp_group.GetValue(str_Game.c_str()));
		LOG("game = %d", game);
		if (!IS_VALID_GAME(game)) continue;

		bool ok = true;
		for (int j = 0; j < num_asm_codes; j++) {
			string from = str_jcs + asm_codes[j].name + str_from;
			string to = str_jcs + asm_codes[j].name + str_to;

			LOG("name : %s", asm_codes[j].name);

			jcs[j].next = GetUIntFromValue(tmp_group.GetValue(from.c_str()));
			jcs[j].to = GetUIntFromValue(tmp_group.GetValue(to.c_str()));

			LOG("from 0x%08X", jcs[j].next);
			LOG("to 0x%08X", jcs[j].to);

			if (jcs[j].next & addr_mark) {
				unsigned addr_next = jcs[j].next & addr_mark;
				byte* p = (byte*)addr_next;
				constexpr int size = 6;
				if (IsBadReadPtr(p, size)) { ok = false; break; };

				if (jcs[j].to >= min_legal_to) {
					int len_op = p[0] == opjmp || p[0] == opcall ? 5 : 6;
					int jc_len = GET_INT(p + len_op - 4);

					unsigned va_to = addr_next + jc_len + len_op;
					LOG("va_to 0x%08X", va_to);
					if (va_to != jcs[j].to) {
						if (asm_codes[j].force){
							ok = false; break;
						}
						else {
							jcs[j].next = jcs[j].to = 0;
						}
					}
				}
			}
			else if(asm_codes[j].force) {
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

	const bool isTiTS = game == GAME_TITS_DX8 || game == GAME_TITS_DX9;
	const bool isSora = game == GAME_SORA || isTiTS;
	const bool isDX8 = game == GAME_SORA || game == GAME_TITS_DX8;
	const bool isDX9 = !isDX8;
	const bool isZa = game == GAME_ZERO || game == GAME_AO;

	unsigned * const addrs = (decltype(addrs))&tp->addrs;
	for (int j = 0; j < num_addr; j++) {
		addrs[j] = GetUIntFromValue(group->GetValue(addr_list[j]));
	}

	const char* comment = group->GetValue(str_Comment.c_str());
	if (comment) {
		for (int i = 0; comment[i] && i < (int)sizeof(tp->Comment) - 1; i++) {
			tp->Comment[i] = comment[i];
		}
		LOG("Data comment, %s", comment);
	}

	memcpy(tp->scodes, isZa ? scode_za : scode_sora, sizeof(scode_za));

	bool suc = false; {
		const char *bin_name = isTiTS ? rc_tits_all : isZa ? rc_za_all : rc_sora_all;
		unique_ptr<RC> rc_bin(RC::Get(bin_name));
		if (rc_bin && rc_bin->First() && rc_bin->Size() < OFF_VLIST - addr_code) {
			memcpy((char*)tp + addr_code, rc_bin->First(), rc_bin->Size());
			suc = true;
		}
	}

	if (!suc) {
		LOG("Read bin failed.");
		return false;
	}
	LOG("Read bin finished.");

	const char* dll_name = isSora && isDX8 ? dll_name_sora : isZa ? dll_name_za : dll_name_sora_dx9;
	HMODULE voice_dll = LoadLibraryA(dll_name);
	if (voice_dll) {
		for (int i = 0; i < NumImport; i++) {
			tp->exps[i] = (unsigned)GetProcAddress(voice_dll, import_names[i]);
			if (!tp->exps[i]) {
				LOG("Symbol not found : %s", import_names[i]);
			}
		}
		LOG("%s loaded", dll_name);
	}
	else {
		LOG("Load dll failed : %s", dll_name);
		return false;
	}

	if (game == GAME_AO) {
		unique_ptr<RC> rc_vlist(RC::Get(rc_ao_vlist));
		if (rc_vlist && rc_vlist->First()) {
			int size = (int)rc_vlist->Size();
			const char* pvlst_rst = rc_vlist->First();
			char* pvlist = (char*)tp + OFF_VLIST;

			for (int i = 0, j = 0; i < size && j < Size - OFF_VLIST - 1; i++) {
				if (pvlst_rst[i] != '\r' && pvlst_rst[i] != '\n') {
					pvlist[j++] = pvlst_rst[i];
				}
				else if (j > 0 && pvlist[j - 1]) {
					j++;
				}
			}
		}
	}

	InitParam *ip = (InitParam*)VirtualAlloc(NULL, Size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

	if (ip) {
		memcpy(ip, tp, Size);
		if (game == GAME_AO) {
			ip->p_rnd_vlst = (char*)ip + OFF_VLIST;
		}
		LOG("Init finished, ip = 0x%08X", (unsigned)ip);
	}
	else {
		LOG("Alloc new memory failed.");
	}

	LOG("Last step.");

	for (int j = 0; j < num_asm_codes; j++) {
		unsigned addr_next = ip->jcs[j].next & addr_mark;
		unsigned addr_type = ip->jcs[j].next >> addr_bits;

		byte* from = (byte*)addr_next;
		if (!from) continue;

		int len_op;
		int add_pmute = 0;
		if (ip->jcs[j].to < min_legal_to) {
			len_op = ip->jcs[j].to & 0xFF;
			add_pmute = (ip->jcs[j].to >> 16) & 0xF;
			int to_add = (int8_t)((ip->jcs[j].to >> 8) & 0xFF);

			if (to_add) {
				ip->jcs[j].to = addr_next + to_add;
			}
			else {
				ip->jcs[j].to = 0;
			}

			if (len_op < 5) len_op = 5;
		}
		else {
			len_op = from[0] == opjmp || from[0] == opcall ? 5 : 6;
		}
		ip->jcs[j].next = addr_next + len_op;

		byte* vs_to = (byte*)ip + asm_codes[j].addr;
		int jc_len = vs_to - from - 5;

		char buff[256];
		memset(buff, opnop, sizeof(buff));

		const byte op = addr_type ? opcall : opjmp;
		PUT(op, buff);
		PUT(jc_len, buff + 1);

		LOG("change code at : 0x%08X", (unsigned)from);
		DWORD dwProtect, dwProtect2;
		if (VirtualProtect(from, len_op, PAGE_EXECUTE_READWRITE, &dwProtect)) {
			if (!ip->addrs.p_mute && add_pmute) {
				ip->addrs.p_mute = *(void**)(from + add_pmute);
			}
			memcpy(from, buff, len_op);
			VirtualProtect(from, len_op, dwProtect, &dwProtect2);
		}
		else {
			LOG("Change code failed.");
		}
	}
	LOG("All done");

	edvoice.dll = voice_dll;
	edvoice.ip = ip;
	for (int i = 0; i < NumImport; i++) {
		if (!strcmp(import_names[i], "Init"))
			edvoice.Init = (decltype(edvoice.Init))ip->exps[i];
		else if (!strcmp(import_names[i], "End"))
			edvoice.End = (decltype(edvoice.End))ip->exps[i];
	}

	if (isSora) {
		edvoice.Init(edvoice.ip);
	}

	return true;
}

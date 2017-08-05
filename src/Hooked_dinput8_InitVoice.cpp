#include "Hooked_dinput8_InitVoice.h"

#include "INI.h"
#include "InitParam.h"
#include "RC.h"
#include "RC_hk_dinput8.h"

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
#define LOG(...) _flog = fopen("ilog.txt", "a+"); fprintf(_flog, __VA_ARGS__), fprintf(_flog, "\n"); fclose(_flog);
#endif // LOG_NOLOG

constexpr int GAME_SORA = 0;
constexpr int GAME_ZERO = 1;
constexpr int GAME_AO = 2;
constexpr int OFF_VLIST = 0xC00;

constexpr char dll_name_sora[] = "ed_voice.dll";
constexpr char dll_name_za[] = "za_voice.dll";

constexpr char rc_SoraData[] = "voice/SoraData.ini";
constexpr char rc_SoraDataEx[] = "voice/SoraDataEx.ini";
constexpr char rc_sora_all[] = "voice/sora_all";
constexpr char rc_za_all[] = "voice/za_all";
constexpr char rc_ao_vlist[] = "voice/ao_rnd_vlst.txt";

constexpr char import_names[][16] = {
	"Init",
	"End",
	"Play",
	"Stop",
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
	"p_global"
};
constexpr int num_addr = sizeof(addr_list) / sizeof(*addr_list);

constexpr const char* name_list[] = {
	"text",
	"dududu",
	"dlgse",
	"aup",
	"scode",
};
constexpr unsigned rvalist[] = {
	0x100,
	0x200,
	0x300,
	0x400,
	0x500
};
constexpr int num_name = sizeof(name_list) / sizeof(*name_list);
constexpr int addr_code = 0x100;

using byte = unsigned char;

constexpr byte opjmp = 0xE9;
constexpr byte opcall = 0xE8;
constexpr byte opnop = 0x90;

constexpr byte scode_sora[] = { 0x54, 0x5B, 0x5C, 0x5D, 0x5E };
constexpr byte scode_za[] = { 0x55, 0x5C, 0x5D, 0x5E, 0x5F };

constexpr int Size = 0x1000;

static bool DoInit(const char* data_name);

bool Init(void* hDll) {
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

	const INI::Group *group = nullptr;
	auto jcs = tp->jcs;
	for (int i = 1; i < ini.Num(); i++) {
		auto& tmp_group = ini.GetGroup(i);
		LOG("Check data: %s", tmp_group.Name());

		bool ok = true;
		for (int j = 0; j < num_name; j++) {
			string from = str_jcs + name_list[j] + str_from;
			string to = str_jcs + name_list[j] + str_to;

			LOG("name : %s", name_list[j]);

			jcs[j].next = GetUIntFromValue(tmp_group.GetValue(from.c_str()));
			jcs[j].to = GetUIntFromValue(tmp_group.GetValue(to.c_str()));

			LOG("from 0x%08X", jcs[j].next);
			LOG("to 0x%08X", jcs[j].to);
			if (!jcs[j].next) { ok = false; break; }
			if (jcs[j].to == 0) continue;

			byte* p = (byte*)jcs[j].next;
			constexpr int size = 6;
			if (IsBadReadPtr(p, size)) { ok = false; break; };

			int len_op = p[0] == opjmp || p[0] == opcall ? 5 : 6;
			int jc_len = GET_INT(p + len_op - 4);

			unsigned va_to = jcs[j].next + jc_len + len_op;
			LOG("va_to 0x%08X", va_to);
			if (va_to != jcs[j].to) { ok = false; break; }
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
		LOG("No mathed data found in ini");
		return false;
	}
	LOG("Data found, %d, %s", group->Num(), group->Name());

	int game = GetUIntFromValue(group->GetValue(str_Game.c_str()));

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

	memcpy(tp->scodes, game ? scode_za : scode_sora, sizeof(scode_za));

	bool suc = false; {
		unique_ptr<RC> rc_bin(RC::Get(game ? rc_za_all : rc_sora_all));
		if (rc_bin && rc_bin->First() && rc_bin->Size() < Size - addr_code) {
			memcpy((char*)tp + addr_code, rc_bin->First(), rc_bin->Size());
			suc = true;
		}
	}

	if (!suc) {
		LOG("Read bin failed.");
		return false;
	}
	LOG("Read bin finished.");

	HMODULE voice_dll = LoadLibraryA(game ? dll_name_za : dll_name_sora);
	if (voice_dll) {
		for (int i = 0; i < NumImport; i++) {
			tp->exps[i] = (unsigned)GetProcAddress(voice_dll, import_names[i]);
			if (!tp->exps[i]) {
				LOG("Symbol not found : %s", import_names[i]);
			}
		}
		LOG("%s loaded", game ? dll_name_za : dll_name_sora);
	}
	else {
		LOG("Load dll failed : %s", game ? dll_name_za : dll_name_sora);
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

	for (int j = 0; j < num_name; j++) {
		byte* from = (byte*)ip->jcs[j].next;

		int len_op = from[0] == opjmp || from[0] == opcall || !ip->jcs[j].to ? 5 : 6;
		ip->jcs[j].next += len_op;

		byte* vs_to = (byte*)ip + rvalist[j];
		int jc_len = vs_to - from - 5;

		char buff[6];
		memset(buff, opnop, sizeof(buff));

		PUT(opjmp, buff);
		PUT(jc_len, buff + 1);

		LOG("change code at : 0x%08X", (unsigned)from);
		DWORD dwProtect, dwProtect2;
		if (VirtualProtect(from, len_op, PAGE_EXECUTE_READWRITE, &dwProtect)) {
			memcpy(from, buff, len_op);
			VirtualProtect(from, len_op, dwProtect, &dwProtect2);
		}
		else {
			LOG("Change code failed.");
		}
	}
	LOG("All done");

	return true;
}

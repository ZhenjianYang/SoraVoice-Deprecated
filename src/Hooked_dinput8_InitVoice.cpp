#include "Hooked_dinput8_InitVoice.h"
#include "ini.h"
#include "rc_hk_dinput8\resource.h"
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

#define SoraData "SORADATA"

FILE* _flog;
#define LOG(...) _flog = fopen("ilog.txt", "a+"); fprintf(_flog, __VA_ARGS__), fprintf(_flog, "\n"); fclose(_flog);

//constexpr char STR_vorbisfile_dll[] = "vorbisfile.dll";
//constexpr char STR_libvorbisfile_dll[] = "libvorbisfile.dll";
//
//constexpr char STR_ov_open_callbacks[] = "ov_open_callbacks";
//constexpr char STR_ov_info[] = "ov_info";
//constexpr char STR_ov_read[] = "ov_read";
//constexpr char STR_ov_clear[] = "ov_clear";
//
//constexpr const char* OggApis[] = {
//	STR_ov_open_callbacks,
//	STR_ov_info,
//	STR_ov_read,
//	STR_ov_clear
//};
//constexpr int NumOggApis = sizeof(OggApis) / sizeof(*OggApis);

constexpr char dll_name_sora[] = "ed_voice.dll";
constexpr char dll_name_za[] = "za_voice.dll";
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
	"p_ov_open_callbacks",
	"p_ov_info",
	"p_ov_read",
	"p_ov_clear",

	"p_d3dd",
	"p_did",
	"p_Hwnd",
	"p_pDS",

	"p_mute",
	"p_keys",
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

constexpr byte opjmp = 0xE9;
constexpr byte opcall = 0xE8;
constexpr byte opnop = 0x90;

constexpr byte scode_sora[] = { 0x54, 0x5B, 0x5C, 0x5D };
constexpr byte scode_za[] = { 0x55, 0x5C, 0x5D, 0x5E };

constexpr int Size = 0x1000;

//void* ov_open_callbacks = nullptr;
//void* ov_info = nullptr;
//void* ov_read = nullptr;
//void* ov_clear = nullptr;
//IDirectSound* pDS = nullptr;

InitParam *ip = nullptr;
static HINSTANCE hd;

static unsigned GetUIntFromValue(const char* str) {
	if (!str || !str[0]) return 0;

	int rad = 10;
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
		rad = 16;
	}
	char *p;
	return std::strtoul(str, &p, rad);
}

void Init(void* hDll)
{
	hd = (HINSTANCE)hDll;
	unique_ptr<byte[]> sp(new byte[Size]);
	memset(sp.get(), 0, Size);
	InitParam* tp = (InitParam*)sp.get();

	INI ini;
	HRSRC rc_ini = FindResourceA(hd, MAKEINTRESOURCE(IDR_SORADATA), SoraData);
	if (rc_ini) {
		HGLOBAL h = LoadResource(hd, rc_ini);
		if (h) {
			int size = SizeofResource(hd, rc_ini);
			char* first = (char*)LockResource(h);
			if (first) {
				stringstream ss(string(first, size));
				ini.Open(ss);
			}
		}
	}

	if (ini.Num() <= 1) {
		LOG("Bad ini file. %d", ini.Num());
		return;
	}
	LOG("ini file opened. %d", ini.Num());
	
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
		return;
	}
	LOG("Data found, %d, %s", group->Num(), group->Name());

	unsigned * const addrs = (decltype(addrs))&tp->addrs;
	for (int j = 0; j < num_addr; j++) {
		addrs[j] = GetUIntFromValue(group->GetValue(addr_list[j]));
	}

	if (!tp->addrs.p_d3dd || !tp->addrs.p_Hwnd) {
		LOG("parameters lost.");
		return;
	}

	bool isZa = GetUIntFromValue(group->GetValue(str_Game.c_str()));

	const char* comment = group->GetValue(str_Comment.c_str());
	if (comment) {
		for (int i = 0; comment[i] && i < (int)sizeof(tp->Comment) - 1; i++) {
			tp->Comment[i] = comment[i];
		}
		LOG("Data comment, %s", comment);
	}

	memcpy(tp->scodes, isZa ? scode_za : scode_sora, sizeof(scode_za));

	HRSRC rc_bin = FindResourceA(hd, MAKEINTRESOURCE(isZa ? IDR_ZA_BIN : IDR_SORA_BIN), SoraData);
	bool suc = false;
	if (rc_bin) {
		HGLOBAL h = LoadResource(hd, rc_bin);
		if (h) {
			int size = SizeofResource(hd, rc_bin);
			void* first = LockResource(h);
			if (first && size < Size - addr_code) {
				memcpy((char*)tp + addr_code, first, size);
				suc = true;
			}
		}
	}

	if (!suc) {
		LOG("Read bin failed.");
		return;
	}
	LOG("Read bin finished.");

//	if (isZa) {
//		HMODULE ogg_dll = LoadLibraryA(STR_vorbisfile_dll);
//		if (!ogg_dll) {
//			ogg_dll = LoadLibraryA(STR_libvorbisfile_dll);
//		}
//
//		if (ogg_dll) {
//			ov_open_callbacks = (void*)GetProcAddress(ogg_dll, STR_ov_open_callbacks);
//			ov_info = (void*)GetProcAddress(ogg_dll, STR_ov_info);
//			ov_read = (void*)GetProcAddress(ogg_dll, STR_ov_read);
//			ov_clear = (void*)GetProcAddress(ogg_dll, STR_ov_clear);
//		}
//
//		if (!ov_open_callbacks || !ov_info || !ov_read || !ov_clear) {
//			LOG("Get ogg api failed.");
//			return;
//		}
//		LOG("ogg loaded.");
//
//		DirectSoundCreate(NULL, &pDS, NULL);
//		if (!pDS || DS_OK != pDS->SetCooperativeLevel((HWND)*tp->addrs.p_Hwnd, DSSCL_PRIORITY)) {
//			LOG("Init DSD failed.");
//			return;
//		};
//		LOG("DSD opened.");
//
//		tp->addrs.p_pDS = (void**)&pDS;
//		tp->addrs.p_ov_open_callbacks = (void**)&ov_open_callbacks;
//		tp->addrs.p_ov_info = (void**)&ov_info;
//		tp->addrs.p_ov_read = (void**)&ov_read;
//		tp->addrs.p_ov_clear = (void**)&ov_clear;
//	}
	

	HMODULE voice_dll = LoadLibraryA(isZa ? dll_name_za : dll_name_sora);
	if (voice_dll) {
		for (int i = 0; i < NumImport; i++) {
			tp->exps[i] = (unsigned)GetProcAddress(voice_dll, import_names[i]);
			if (!tp->exps[i]) {
				LOG("Symbol not found : %s", import_names[i]);
			}
		}
	}
	else {
		LOG("Load dll failed : %s", isZa ? dll_name_za : dll_name_sora);
	}
	LOG("%s loaded", isZa ? dll_name_za : dll_name_sora);

	ip = (InitParam*)VirtualAlloc(NULL, Size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

	if (ip) {
		memcpy(ip, tp, Size);
		LOG("Init finished, ip = 0x%08X", (unsigned)ip);
	}
	else {
		LOG("Alloc new memory failed.");
	}
}

void Go()
{
	if (!ip) return;
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
}

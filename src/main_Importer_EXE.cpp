#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <iomanip>
#include <algorithm>

#include "Common.h"
#include "InitParam.h"
#include "INI.h"

using namespace std;
using byte = unsigned char;

const int Align = 0x1000;
const int Base = 0x400000;

const int OFF_E_lfanew = 0x3C;

const int ROFF_SizeInitData = 28;
const int ROFF_SizeTotal = 76;
const int ROFF_RVAImportTable = 124;
const int ROFF_SectionInfos = 244;

const int NewSecFlag = 0xE0000060;

const int ROFF_DLLName = 0x140;
const int ROFF_Lookup = ROFF_DLLName + 0x10;
const int ROFF_Names = ROFF_Lookup + 0x20;

const string nsec_name = ".voice";

const char dll_name_sora[] = "ed_voice.dll";
const char dll_name_za[] = "za_voice.dll";
const char import_names[][16] = {
	"Init",
	"End",
	"Play",
	"Stop",
};
const int NumImport = sizeof(import_names) / sizeof(*import_names);

const string str_jcs = "jcs_";
const string str_from = "_from";
const string str_to = "_to";
const string str_Comment = "Comment";
const string str_Game = "Game";

const byte opjmp = 0xE9;
const byte opcall = 0xE8;
const byte opnop = 0x90;

const byte scode_sora[] = { 0x54, 0x5B, 0x5C, 0x5D };
const byte scode_za[]   = { 0x55, 0x5C, 0x5D, 0x5E };

static unsigned GetUIntFromValue(const char* str) {
	if (!str || !str[0]) return 0;

	int rad = 10;
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
		rad = 16;
	}
	char *p;
	return std::strtoul(str, &p, rad);
}

const char addr_list[][20] = {
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
const int num_addr = sizeof(addr_list) / sizeof(*addr_list);

const char nmlist[][10] = {
	"text",
	"dududu",
	"dlgse",
	"aup",
	"scode",
};
const unsigned rvalist[] = {
	0x100,
	0x200,
	0x300,
	0x400,
	0x500
};
const int num_name = sizeof(nmlist) / sizeof(*nmlist);

const int roff_ip = 0x200;

static_assert(sizeof(import_names[0]) % 2 == 0, "Size of import_name must be even");

#define GET_INT(ptr) *(int*)(ptr)
#define GET_SHORT(ptr) *(short*)(ptr)

#define PUT(var, ptr) memcpy(ptr, &var, sizeof(var))
#define PUT_ARRAY(arr, ptr) memcpy(ptr, arr, sizeof(arr))

int main(int argc, char* argv[])
{
	if (argc <= 5) {
		cout << "Usage:\n"
			"\t" "Importer_EXE.exe EXE_new EXE_old SoraData.ini bin report\n"
			<< endl;
		return 0;
	}
	
	const string path_exe_new = argv[1];
	const string path_exe_old = argv[2];
	const string path_data = argv[3];
	const string path_bin = argv[4];
	const string path_report = argv[5];

	ifstream ifs(path_exe_old, ios::binary);
	if (!ifs) {
		cout << "Open file failed, exit: " << path_exe_old << endl;
		return 0;
	}
	ifs.seekg(0, ios::end);
	int len_old = (int)ifs.tellg();
	ifs.seekg(0, ios::beg);
	unique_ptr<byte[]> sbuff_old(new byte[len_old]);
	ifs.read((char*)sbuff_old.get(), len_old);
	const byte* const buff = sbuff_old.get();
	ifs.close();

	const int E_lfanew = GET_INT(buff + OFF_E_lfanew);
	const int off_Head = E_lfanew + 4;

	const int off_NumSections = off_Head + 2;
	const int off_SizeInitData = off_Head + ROFF_SizeInitData;
	const int off_SizeTotal = off_Head + ROFF_SizeTotal;
	const int off_RVAImportTable = off_Head + ROFF_RVAImportTable;
	const int off_SizeImportTable = off_RVAImportTable + 4;
	const int off_SectionInfos = off_Head + ROFF_SectionInfos;
	
	const short NumSections = GET_SHORT(buff + off_NumSections);
	const int SizeInitData = GET_INT(buff + off_SizeInitData);
	const int SizeTotal = GET_INT(buff + off_SizeTotal);
	const int RVAImportTable = GET_INT(buff + off_RVAImportTable);
	const int SizeImportTable = GET_INT(buff + off_SizeImportTable);

	struct SectionInfo
	{
		char name[8];
		int vSize;
		int vAddr;
		int Size;
		int Off;
		char nouse[12];
		int flag;
	};
	SectionInfo *psi = (SectionInfo *)(buff + off_SectionInfos);

	bool added = false;
	int Off_end = 0;
	int vOff_end = 0;

	SectionInfo si_new;
	memset(&si_new, 0, sizeof(si_new));
	vector<SectionInfo> sis;

	int Off_sec_new = 0;
	for(;psi->name[0]; psi++)
	{
		sis.push_back(*psi);

		string name_tmp;
		if (psi->name[sizeof(psi->name) - 1]) name_tmp.assign(psi->name, sizeof(psi->name) - 1);
		else name_tmp.assign(psi->name);

		if (name_tmp == nsec_name) {
			added = true;
			si_new = *psi;
			Off_sec_new = (byte*)psi - buff;
		}

		Off_end = std::max(Off_end, psi->Off + psi->Size);
		vOff_end = std::max(vOff_end, psi->vAddr + psi->vSize);
	}

	auto GetOffFromRVA = [&sis](int rva) -> int {
		for (const auto &si : sis) {
			if (si.vAddr <= rva && rva < si.vAddr + si.vSize) {
				return rva - si.vAddr + si.Off;
			}
		}
		return 0;
	};

	const int off_ImportTable = GetOffFromRVA(RVAImportTable);

	short NumSections_new = NumSections;
	int SizeInitData_new = SizeInitData;
	int SizeTotal_new = SizeTotal;

	int len_new;
	if (!added) {
		Off_sec_new = (byte*)psi - buff;
		Off_end = (Off_end + Align - 1) / Align * Align;
		vOff_end = (vOff_end + Align - 1) / Align * Align;

		int nsec_size = Align;
		len_new = Off_end + nsec_size;

		for (int i = 0; i < sizeof(si_new.name) && i < (int)nsec_name.size(); i++) si_new.name[i] = nsec_name[i];
		si_new.vSize = nsec_size;
		si_new.vAddr = vOff_end;
		si_new.Size = nsec_size;
		si_new.Off = Off_end;

		len_new = si_new.Off + si_new.Size;

		NumSections_new++;
		SizeInitData_new += nsec_size;
		SizeTotal_new += nsec_size;

		sis.push_back(si_new);
	}
	si_new.flag = NewSecFlag;

	unique_ptr<byte[]> sbuff_new(new byte[len_new]);
	byte* const buff_new = sbuff_new.get();
	for (int i = 0; i < len_old; i++) buff_new[i] = buff[i];
	
	memset(buff_new + si_new.Off, 0, si_new.Size);
	for (int i = 0; i < SizeImportTable; i++) {
		buff_new[si_new.Off + i] = buff[off_ImportTable + i];
	}

	INI ini(path_data.c_str());
	if (!ini.Valid() || ini.Num() <= 1) {
		cout << "Bad data file, return: " << path_data << endl;
		return 0;
	}

	const INI::Group *group = nullptr;
	struct {
		unsigned from;
		unsigned to;
	} jcs[num_name];
	for (int i = 1; i < ini.Num(); i++) {
		auto& tmp = ini.GetGroup(i);
		memset(&jcs, 0, sizeof(jcs));

		bool ok = true;
		for (int j = 0; j < num_name; j++) {
			string from = str_jcs + nmlist[j] + str_from;
			string to = str_jcs + nmlist[j] + str_to;

			jcs[j].from = GetUIntFromValue(tmp.GetValue(from.c_str()));
			jcs[j].to = GetUIntFromValue(tmp.GetValue(to.c_str()));

			if (!jcs[j].from) { ok = false; break; }
			if (jcs[j].to == 0) continue;

			unsigned off_from = GetOffFromRVA(jcs[j].from - Base);
			if(off_from > (unsigned)len_old) { ok = false; break; }
			int len_op = buff[off_from] == opjmp || buff[off_from] == opcall ? 5 : 6;
			int jc_len = GET_INT(buff + off_from + len_op - 4);

			unsigned va_to = jcs[j].from + jc_len + len_op;
			if(va_to != jcs[j].to) { ok = false; break; }
		}

		if (ok) {
			group = &tmp;
			break;
		}
	}

	if (!group) {
		cout << "No matching data found, exit." << endl;
		return 0;
	}

	unsigned addrs[num_addr];
	for (int j = 0; j < num_addr; j++) {
		addrs[j] = GetUIntFromValue(group->GetValue(addr_list[j]));
	}
	bool isZa = GetUIntFromValue(group->GetValue(str_Game.c_str()));

	const char* comment = group->GetValue(str_Comment.c_str());

	for (int j = 0; j < num_name; j++) {
		unsigned off_from = GetOffFromRVA(jcs[j].from - Base);
		int len_op = buff[off_from] == opjmp || buff[off_from] == opcall || !jcs[j].to ? 5 : 6;

		unsigned vs_to = si_new.vAddr + Base + roff_ip + rvalist[j];

		int jc_len = vs_to - jcs[j].from - 5;

		PUT(opjmp, buff_new + off_from);
		PUT(jc_len, buff_new + off_from + 1);

		if (len_op == 6) {
			PUT(opnop, buff_new + off_from + 5);
		}

		jcs[j].from += len_op;
	}

	ifstream ifs_bin(path_bin, ios::binary);
	if (!ifs_bin) {
		cout << "Open file failed, exit: " << path_bin << endl;
		return 0;
	}
	ifs_bin.seekg(0, ios::end);
	int len_bin = (int)ifs_bin.tellg();
	ifs_bin.seekg(0, ios::beg);
	ifs_bin.read((char*)(buff_new + si_new.Off + roff_ip + rvalist[0]), len_bin);
	ifs_bin.close();

	InitParam* ip = (InitParam*)(buff_new + si_new.Off + roff_ip);
	memset(ip, 0, sizeof(*ip));

	memcpy(ip->scodes, isZa ? scode_za : scode_sora, sizeof(scode_za));
	memcpy(ip->jcs, jcs, sizeof(jcs));
	memcpy(&ip->addrs, addrs, sizeof(addrs));
	if (comment) {
		for (int i = 0; comment[i] && i < sizeof(ip->Comment) - 1; i++) {
			ip->Comment[i] = comment[i];
		}
	}

	struct ImportTalbeInfo
	{
		int RVA_Lookup;
		int data1;
		int data2;
		int RVA_Name;
		int RVA_Import;
	};
	ImportTalbeInfo *iti = (ImportTalbeInfo*)(buff + off_ImportTable);
	ImportTalbeInfo iti_new;
	memset(&iti_new, 0, sizeof(iti_new));
	int roff_iti_new = 0;
	bool imported = false;

	for(;iti->RVA_Lookup; iti++)
	{
		int off_name = GetOffFromRVA(iti->RVA_Name);

		if(strcmp(dll_name_sora, (const char*)(buff + off_name)) == 0
			|| strcmp(dll_name_za, (const char*)(buff + off_name)) == 0){
			imported = true;
			roff_iti_new = (byte*)iti - (buff + off_ImportTable);
		}
	}
	int RVAImportTable_new = si_new.vAddr;
	int SizeImportTable_new = (byte*)iti - (buff + off_ImportTable) + sizeof(ImportTalbeInfo);

	if (!imported) {
		roff_iti_new = (byte*)iti - (buff + off_ImportTable);
		SizeImportTable_new += sizeof(ImportTalbeInfo);
	}

	int off_iti_new = roff_iti_new + si_new.Off;

	iti_new.RVA_Name = si_new.vAddr + ROFF_DLLName;
	iti_new.RVA_Lookup = si_new.vAddr + ROFF_Lookup;
	iti_new.RVA_Import = si_new.vAddr + roff_ip + ((char*)ip->exps - (char*)ip);

	int lookUp[NumImport];
	int imp[NumImport];

	for (int i = 0; i < NumImport; i++) {
		lookUp[i] = imp[i] = si_new.vAddr + ROFF_Names + (import_names[i] - import_names[0]);
	}
	if(isZa) PUT_ARRAY(dll_name_za, buff_new + si_new.Off + ROFF_DLLName);
	else PUT_ARRAY(dll_name_sora, buff_new + si_new.Off + ROFF_DLLName);
	PUT_ARRAY(import_names, buff_new + si_new.Off + ROFF_Names + 2);
	PUT_ARRAY(lookUp, buff_new + si_new.Off + ROFF_Lookup);
	PUT_ARRAY(imp, ip->exps);

	PUT(iti_new, buff_new + off_iti_new);
	PUT(si_new, buff_new + Off_sec_new);
	
	PUT(RVAImportTable_new, buff_new + off_RVAImportTable);
	PUT(SizeImportTable_new, buff_new + off_SizeImportTable);
	PUT(NumSections_new, buff_new + off_NumSections);
	PUT(SizeInitData_new, buff_new + off_SizeInitData);
	PUT(SizeTotal_new, buff_new + off_SizeTotal);

	if (!path_report.empty()) {
		ofstream ofsr(path_report);

		if (ofsr) {
			ofsr << nsec_name << '\n'
				<< "VA = 0x" << setfill('0') << setw(8) << setiosflags(ios::right) << hex << setiosflags(ios::uppercase) << si_new.vAddr + Base << '\n'
				<< "RVA = 0x" << setfill('0') << setw(8) << setiosflags(ios::right) << hex << setiosflags(ios::uppercase) << si_new.vAddr << '\n'
				<< "Offset = 0x" << setfill('0') << setw(8) << setiosflags(ios::right) << hex << setiosflags(ios::uppercase) << si_new.Off << '\n'
				<< "Size = 0x" << setfill('0') << setw(4) << setiosflags(ios::right) << hex << setiosflags(ios::uppercase) << si_new.Size << '\n'
				<< "Comment = " << (comment ? comment : "") << endl;

			ofsr.close();
		}
		else {
			cout << "Creat file failed: " << path_report << endl;
		}
	}

	ofstream ofs(path_exe_new, ios::binary);
	ofs.write((const char*)buff_new, len_new);
	ofs.close();

	return 0;
}

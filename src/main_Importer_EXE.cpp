#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <iomanip>
#include <algorithm>

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
const int ROFF_Import = ROFF_Lookup + 0x20;
const int ROFF_Names = ROFF_Import + 0x20;

const string nsec_name = ".voice";

const char dll_name[] = "ed_voice.dll";
const char import_names[][16] = {
	"_Init@4",
	"_End@4",
	"_Play@8",
	"_Stop@4",
	"_Input@4",
};
const int NumImport = sizeof(import_names) / sizeof(*import_names);

const char str_define[] = "%define";

const string str_addr_jmp = "addr_jmp_";
const string str_addr_jmpto = "addr_jmpto_";
const string str_addr_call = "addr_call_";
const string str_addr_callto = "addr_";
//const string str_addr_callapi = "addr_callapi_";
//const string str_addr_api = "addr_api_";

const byte opjmp = 0xE9;
const byte opcall = 0xE8;

const map<string, int> map_ptr_roff = {
	{"p_ov_open_callbacks",0x18 },
	{"p_ov_info",0x1C},
	{"p_ov_read",0x20},
	{"p_ov_clear",0x24},

	{"p_d3dd",0x40},
	{"p_hwnd",0x44},
	{"p_pDS",0x48},
	{"p_keys",0x4C}
};

const map<string, int> map_vs_roff = {
	{ "text",0x00 },
	{ "dududu",0x80 },
	{ "dlgse",0x100 },
	{ "input",0x1C0 },
};
const int roff_base_ptr = 0x240;
const int roff_base_vs = roff_base_ptr + 0xC0;

static_assert(sizeof(import_names[0]) % 2 == 0, "Size of import_name must be even");

#define GET_INT(ptr) *(int*)(ptr)
#define GET_SHORT(ptr) *(short*)(ptr)

#define PUT(var, ptr) *(decltype(var)*)(ptr) = var
#define PUT_ARRAY(arr, ptr) memcpy(ptr, arr, sizeof(arr))

static map<string, int> GetMacroValues(const string& path);

int main(int argc, char* argv[])
{
	if (argc <= 2) {
		cout << "Usage:\n"
			"\t" "Importer_EXE.exe EXE_new EXE_old [-m macro_file] [-r report] [-b bin1 bin2 bin3...]\n"
			<< endl;
		return 0;
	}
	
	string path_exe_new = argv[1];
	string path_exe_old = argv[2];

	enum class Flag { Macro, Report, BIN, None };

	string path_macro;
	string path_report;
	vector<string> path_bins;

	Flag flag = Flag::None;
	for (int i = 3; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch (argv[i][1])
			{
			case 'm':
				flag = Flag::Macro;
				break;
			case 'r':
				flag = Flag::Report;
				break;
			case 'b':
				flag = Flag::BIN;
				break;
			default:
				flag = Flag::None;
				cout << "Ignore Unknown parameter: " << argv[i] << endl;
				break;
			}
		}
		else {
			switch (flag)
			{
			case Flag::Macro:
				if (path_macro.empty()) path_macro = argv[i];
				else cout << "Ignore parameter has been set: " << argv[i] << endl;
				break;
			case Flag::Report:
				if (path_report.empty()) path_report = argv[i];
				else cout << "Ignore parameter has been set: " << argv[i] << endl;
				break;
			case Flag::BIN:
				path_bins.push_back(argv[i]);
				break;
			case Flag::None: default:
				cout << "Ignore unvailid parameter: " << argv[i] << endl;
				break;
			}
		}
	}

	ifstream ifs(path_exe_old, ios::binary);
	if (!ifs) {
		cout << "Open file failed, exit: " << path_exe_old << endl;
		return 0;
	}
	ifs.seekg(0, ios::end);
	int len_old = (int)ifs.tellg();
	ifs.seekg(0, ios::beg);
	unique_ptr<byte> sbuff_old(new byte[len_old]);
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

	unique_ptr<byte> sbuff_new(new byte[len_new]);
	byte* const buff_new = sbuff_new.get();
	for (int i = 0; i < len_old; i++) buff_new[i] = buff[i];
	
	memset(buff_new + si_new.Off, 0, si_new.Size);
	for (int i = 0; i < SizeImportTable; i++) {
		buff_new[si_new.Off + i] = buff[off_ImportTable + i];
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

		if(strcmp(dll_name, (const char*)(buff + off_name)) == 0){
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
	iti_new.RVA_Import = si_new.vAddr + ROFF_Import;

	int lookUp[NumImport];
	int imp[NumImport];

	for (int i = 0; i < NumImport; i++) {
		lookUp[i] = imp[i] = si_new.vAddr + ROFF_Names + (import_names[i] - import_names[0]);
	}
	PUT_ARRAY(dll_name, buff_new + si_new.Off + ROFF_DLLName);
	PUT_ARRAY(import_names, buff_new + si_new.Off + ROFF_Names + 2);
	PUT_ARRAY(lookUp, buff_new + si_new.Off + ROFF_Lookup);
	PUT_ARRAY(imp, buff_new + si_new.Off + ROFF_Import);

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
				<< "Size = 0x" << setfill('0') << setw(4) << setiosflags(ios::right) << hex << setiosflags(ios::uppercase) << si_new.Size << '\n';

			ofsr.close();
		}
		else {
			cout << "Creat file failed: " << path_report << endl;
		}
	}

	if (!path_macro.empty()) {
		auto mv = GetMacroValues(path_macro);

		for (const auto& it : mv) {
			if (it.first.find(str_addr_jmp) == 0) {
				string sn = it.first.substr(1 + it.first.rfind('_'));
				auto it2 = map_vs_roff.find(sn);
				if (it2 == map_vs_roff.end()) continue;

				int rva_jmp = it.second - Base;
				int rva_jmpto = it2->second + roff_base_vs + si_new.vAddr;

				int len_jmp = rva_jmpto - rva_jmp - 5;
				int off_jmp = GetOffFromRVA(rva_jmp);

				if (buff[off_jmp] != opjmp && buff[off_jmp] != opcall) {
					off_jmp++;
					len_jmp--;
				}
				PUT(len_jmp, buff_new + off_jmp + 1);
			}
			else if (it.first.find(str_addr_call) == 0) {
				string sn = it.first.substr(str_addr_call.size());
				auto it2 = map_vs_roff.find(sn);
				if (it2 == map_vs_roff.end()) continue;

				int rva_call = it.second - Base;
				int rva_callto = it2->second + roff_base_vs + si_new.vAddr;

				int len_call = rva_callto - rva_call - 5;
				int off_call = GetOffFromRVA(rva_call);
				byte code = opjmp;

				PUT(code, buff_new + off_call);
				PUT(len_call, buff_new + off_call + 1);
			}
		}

		for(auto & it : map_ptr_roff) {
			int roff = it.second;
			int off = roff + roff_base_ptr + si_new.Off;

			auto it2 = mv.find(it.first);
			if (it2 == mv.end() || it2->second == 0) continue;

			int value = it2->second;
			PUT(value, buff_new + off);
		}
	}

	if (!path_bins.empty()) {
		for (const auto& path_bin : path_bins) {
			string name_bin = path_bin.substr(path_bin.rfind('\\') + 1);
			
			for (const auto& vs_roff : map_vs_roff) {
				if (name_bin.find(vs_roff.first) != string::npos) {
					ifstream ifs_bin(path_bin, ios::binary);
					if (!ifs_bin) {
						cout << "Open file failed, skip: " << path_bin << endl;
						continue;
					}

					ifs_bin.seekg(0, ios::end);
					int len_bin = (int)ifs_bin.tellg();
					ifs_bin.seekg(0, ios::beg);
					unique_ptr<byte> sbuff_bin(new byte[len_bin]);
					ifs_bin.read((char*)sbuff_bin.get(), len_bin);
					const byte* const buff_bin = sbuff_bin.get();
					ifs_bin.close();

					int off = roff_base_vs + vs_roff.second + si_new.Off;
					for (int i = 0; i < len_bin; i++) {
						buff_new[off + i] = buff_bin[i];
					}
					int fake = 0xCCCCCCCC;
					PUT(fake, buff_new + off - 4);

					break;
				}
			}
		}
	}

	ofstream ofs(path_exe_new, ios::binary);
	ofs.write((const char*)buff_new, len_new);
	ofs.close();

	return 0;
}

map<string, int> GetMacroValues(const string& path) {
	map<string, int> rst;
	ifstream ifs(path);
	char buff[1024];

	while (ifs.getline(buff, sizeof(buff)))
	{
		int idx = 0;
		while (buff[idx] && str_define[idx] == buff[idx]) idx++;
		if (str_define[idx]) continue;

		while (buff[idx] == ' ' || buff[idx] == '\t') idx++;

		string key;
		while (buff[idx] && buff[idx] != ' ' && buff[idx] != '\t') key.push_back(buff[idx++]);
		if (buff[idx] != ' ' && buff[idx] != '\t') continue;

		while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) key.pop_back();
		if (key.empty()) continue;

		while (buff[idx] == ' ' || buff[idx] == '\t') idx++;
		int rad = 10;
		if (buff[idx] == '0' && (buff[idx + 1] == 'x' || buff[idx + 1] == 'X')) rad = 16;
		char* p;
		unsigned value = std::strtoul(buff + idx, &p, rad);

		rst[key] = (int)value;
	}

	return rst;
}


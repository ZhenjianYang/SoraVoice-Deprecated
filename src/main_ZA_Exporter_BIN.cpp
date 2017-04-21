#include <iostream>
#include <fstream>
#include <iomanip>
#include <memory>

#include "common.h"

#define ATTR_BIN ".bin"
#define ATTR_OUT ".txt"
#define REP_NAME "bin_report.txt"

#define GET_U32(ptr) *(unsigned*)(ptr)
#define GET_U16(ptr) *(unsigned short*)(ptr)

#define OFF_OFF_STRING 0x34
#define OFF_OFF_FUN_TABLE 0x42

using namespace std;

using byte = unsigned char;

int main(int argc, char* argv[])
{
	if (argc <= 1) {
		cout << "Usage:\n"
			"\t" "ZA_Exporter_BIN dir_bin [dir_out]\n"
			"\n"
			"Default: dir_out = dir_bin.out"
			<< endl;
		return 0;
	}

	string dir_bin = argv[1];  while (dir_bin.back() == '/' || dir_bin.back() == '\\') dir_bin.pop_back();
	string dir_out = argc > 2 ? argv[2] : dir_bin + ".out";

	Sora::MakeDirectory(dir_out);
	if (dir_out.length() > 0 && dir_out.back() != '\\') dir_out.push_back('\\');

	dir_bin.push_back('\\');
	vector<string> fn_bins;
	Sora::SearchFiles(dir_bin + "*" ATTR_BIN, fn_bins);

	ofstream ofs_rp(REP_NAME);
	for (const auto &fn_bin : fn_bins) {
		const string name = fn_bin.substr(0, fn_bin.rfind(ATTR_BIN));
		cout << "处理" << fn_bin << "..." << endl;

		ifstream ifs(dir_bin + fn_bin, ios::binary);
		ofstream ofs;
		int out_cnt = 0;
		
		ifs.seekg(0, ios::end);
		int len = (int)ifs.tellg();
		std::unique_ptr<byte[]> sbuff(new byte[len]);
		ifs.seekg(0, ios::beg);
		ifs.read((char*)sbuff.get(), len);
		ifs.close();
		const byte* const buff = sbuff.get();

		unsigned off_string = GET_U32(buff + OFF_OFF_STRING);
		unsigned off_fun_table = GET_U16(buff + OFF_OFF_FUN_TABLE);
		unsigned size_fun_table = GET_U16(buff + OFF_OFF_FUN_TABLE + 2);

		int fun_cnt = size_fun_table / 4;
		vector<unsigned> funs;
		for (int i = 0; i < fun_cnt; i++) {
			funs.push_back(GET_U32(buff + off_fun_table + i * 4));
		}
		funs.push_back(off_string);

		for (int i = 0; i < fun_cnt; i++) {
			unsigned start = funs[i];
			unsigned end = funs[i+1];
		}

		ofs_rp << name << '\t' <<  out_cnt << endl;
	}
	ofs_rp.close();

	return 0;
}

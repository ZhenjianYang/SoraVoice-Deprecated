#include <iostream>
#include <fstream>
#include <iomanip>
#include <memory>

#include "common.h"

#define ATTR_MBIN ".mbin"
#define ATTR_OUT ".txt"
#define REP_NAME "mbin_report.txt"


#define GET_INT(ptr) *(int*)(ptr)

using namespace std;

using byte = unsigned char;

int main(int argc, char* argv[])
{
	if (argc <= 1) {
		cout << "Usage:\n"
			"\t" "Exporter_MBIN dir_mbin [dir_out]\n"
			"\n"
			"Default: dir_out = dir_mbin.out"
			<< endl;
		return 0;
	}

	string dir_mbin = argv[1];  while (dir_mbin.back() == '/' || dir_mbin.back() == '\\') dir_mbin.pop_back();
	string dir_out = argc > 2 ? argv[2] : dir_mbin + ".out";

	Sora::MakeDirectory(dir_out);
	if (dir_out.length() > 0 && dir_out.back() != '\\') dir_out.push_back('\\');

	dir_mbin.push_back('\\');
	vector<string> fn_mbins;
	Sora::SearchFiles(dir_mbin + "*" ATTR_MBIN, fn_mbins);

	ofstream ofs_rp(REP_NAME);
	for (const auto &fn_mbin : fn_mbins) {
		const string name = fn_mbin.substr(0, fn_mbin.rfind(ATTR_MBIN));
		cout << "´¦Àí" << fn_mbin << "..." << endl;

		ifstream ifs(dir_mbin + fn_mbin, ios::binary);
		ofstream ofs;
		int out_cnt = 0;
		
		ifs.seekg(0, ios::end);
		int len = (int)ifs.tellg();
		std::unique_ptr<byte> sbuff(new byte[len]);
		byte* buff = sbuff.get();
		ifs.seekg(0, ios::beg);
		ifs.read((char*)buff, len);
		ifs.close();

		int num = GET_INT(buff);
		const int base = 4 + num * 8;

		for (int i = 0; i < num; i++) {
			int type = GET_INT(buff + 4 + i * 8);
			int offset = GET_INT(buff + 4 + i * 8 + 4);
			int offset_next = i == num - 1 ? len - base: GET_INT(buff + 4 + i * 8 + 12);

			int start = offset + base;
			int end = offset_next + base;

			int j = start;
			int cnt = 0;
			while (j < end)
			{
				while (j < end && buff[j] != '#') j++;
				int t = j;
				j++;
				while (j < end && buff[j] >= '0' && buff[j] <= '9') j++;

				if (j < end && buff[j] == 'J') {
					j = t;
					cnt++; 
					string text;
					while (j < end) {
						if (j < end - 1 && buff[j] == 2 && (buff[j + 1] == 3 || buff[j + 1] == 0)) break;

						if (buff[j] == 1) text.append("\\n");
						else if(buff[j] >= 0x20 && buff[j] < 0xFF) text.push_back(buff[j]);
						j++;
					}

					if (out_cnt == 0) {
						ofs.open(dir_out + name + ATTR_OUT);
					}
					int line_no = t * 0x10 + type;
					ofs << setfill('0') << setw(6) << setiosflags(ios::right) << hex << setiosflags(ios::uppercase) << line_no << ","
						<< text << "\n\n";
					out_cnt++;
				}
			}
		}
		if(out_cnt > 0) ofs.close();

		ofs_rp << name << '\t' <<  out_cnt << endl;
	}
	ofs_rp.close();

	return 0;
}

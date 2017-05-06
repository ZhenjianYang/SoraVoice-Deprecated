#include <iostream>
#include <fstream>
#include <iomanip>
#include <assert.h>

#include "Common.h"
#include "SoraIE.h"

#define ATTR_SNT "._SN.txt"

using namespace std;

const string mark_opA = "op#A";
const string mark_opW = "op#W";
const string mark_op5 = "op#5";

int main(int argc, char* argv[])
{
	if (argc <= 1) {
		cout << "Usage:\n"
			"\t" "Foramt_SNT dir_snt [dir_out]\n"
			"\n"
			"Default: dir_out = dir_snt.fmt"
			<< endl;
		return 0;
	}

	string dir_snt = argv[1];  while (dir_snt.back() == '/' || dir_snt.back() == '\\') dir_snt.pop_back();
	string dir_out = argc > 2 ? argv[2] : dir_snt + ".fmt";

	Sora::MakeDirectory(dir_out);
	if (dir_out.length() > 0 && dir_out.back() != '\\') dir_out.push_back('\\');

	dir_snt.push_back('\\');
	vector<string> fn_snts;
	Sora::SearchFiles(dir_snt + "*" ATTR_SNT, fn_snts);
	for (const auto &fn_snt : fn_snts) {
		const string name = fn_snt.substr(0, fn_snt.rfind(ATTR_SNT));
		cout << "处理" << fn_snt << "..." << endl;

		ifstream ifs(dir_snt + fn_snt);
		SoraSNT snt(ifs);
		ifs.close();
		
		ofstream ofs(dir_out + fn_snt);
		int cnt_talk = 0;
		for (int i = 0; i < snt.Num(); i++) {
			const auto& item = snt[i];

			if (item.Type != AllItemTypes::Nomarl) {
				cnt_talk++;
				ofs << ";" << snt[i].Type->Mark
					<< setfill('0') << setw(4) << setiosflags(ios::right) << cnt_talk;

				bool has_opA = false;
				bool has_opW = false;
				bool has_op5 = false;

				for (size_t j = item.Type->TextStartLine + 1; j < item.Num() - 1; j++) {
					const string& line = item[j].content;

					if (line.find("\\5") != string::npos) has_op5 = true;
					size_t k = 0;
					while (k < line.length()) {
						while (k < line.length() && line[k] != '#') k++;
						if(line[k] == '#')  k++;
						while (line[k] >= '0' && line[k] <= '9') k++;
						if (line[k] == 'A') has_opA = true;
						if (line[k] == 'W') has_opW = true;
						k++;
					}
				}

				if (has_opA) ofs << ' ' << mark_opA;
				if (has_opW) ofs << ' ' << mark_opW;
				if (has_op5) ofs << ' ' << mark_op5;

				ofs << '\n';
			}

			item.Output(ofs);
		}
		ofs.close();

	}

	return 0;
}

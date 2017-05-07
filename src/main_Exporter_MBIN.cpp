#include <iostream>
#include <fstream>
#include <iomanip>
#include <memory>

#include "Common.h"
#include "SoraIE.h"

#define ATTR_MBIN ".mbin"
#define ATTR_OUT ".txt"
#define REP_NAME "mbin_report.txt"


#define GET_INT(ptr) *(int*)(ptr)

using namespace std;
constexpr char Seperator[] = "#-------------------------------------------------------------------#";

int main(int argc, char* argv[])
{
	if (argc <= 1) {
		cout << "Usage:\n"
			"\t" "Exporter_MBIN [-utf8] dir_mbin [dir_out]\n"
			"\t\t" "-utf8 utf8 encode"
			"\n"
			"Default: dir_out = dir_mbin.out"
			<< endl;
		return 0;
	}

	bool utf8 = string("-utf8") == argv[1];
	int iarg = utf8 ? 2 : 1;

	string dir_mbin = argv[iarg + 0];  while (dir_mbin.back() == '/' || dir_mbin.back() == '\\') dir_mbin.pop_back();
	string dir_out = argc > iarg + 1 ? argv[iarg + 1] : dir_mbin + ".out";

	Sora::MakeDirectory(dir_out);
	if (dir_out.length() > 0 && dir_out.back() != '\\') dir_out.push_back('\\');

	dir_mbin.push_back('\\');
	vector<string> fn_mbins;
	Sora::SearchFiles(dir_mbin + "*" ATTR_MBIN, fn_mbins);

	ofstream ofs_rp(REP_NAME);
	for (const auto &fn_mbin : fn_mbins) {
		const string name = fn_mbin.substr(0, fn_mbin.rfind(ATTR_MBIN));
		cout << "处理" << fn_mbin << "..." << endl;

		int out_cnt = 0;

		const auto mbinTalks = MbinTalk::GetMbinTalks(dir_mbin + fn_mbin, utf8);
		if (mbinTalks.size() > 0) {
			ofstream ofs(dir_out + name + ATTR_OUT);

			for (const auto& talk : mbinTalks) {
				ofs << Seperator << "\n" << Seperator << "\n\n";

				bool op2 = false;
				for (size_t i = 0; i < talk.Texts.size(); i++) {
					if (op2) ofs << '\n';
					op2 = talk.Texts[i].text.find(R"(\2)") != string::npos;

					ofs << talk.Type->Mark
						<< setfill('0') << setw(4) << setiosflags(ios::right) << talk.ID << ","
						<< setfill('0') << setw(2) << setiosflags(ios::right) << i + talk.Type->TextStartLine + 1 << ","
						<< talk.Texts[i].text
						<< "\n\n";

					out_cnt++;
				}
			}
		}

		ofs_rp << name << '\t' << mbinTalks.size() << '\t' << out_cnt << endl;
	}
	ofs_rp.close();

	return 0;
}

#include <iostream>
#include <fstream>
#include <iomanip>
#include <assert.h>

#include "Common.h"
#include "SoraSNT.h"

#define ATTR_SNT "._SN.txt"
#define ATTR_OUT ".txt"
#define REP_NAME "snt_report.txt"

constexpr char Seperator[] = "#-------------------------------------------------------------------#";

using namespace std;

int main(int argc, char* argv[])
{
	if (argc <= 1) {
		cout << "Usage:\n"
			"\t" "Exporter_SNT dir_snt [dir_out]\n"
			"\n"
			"Default: dir_out = dir_snt.out"
			<< endl;
		return 0;
	}

	string dir_snt = argv[1];  while (dir_snt.back() == '/' || dir_snt.back() == '\\') dir_snt.pop_back();
	string dir_out = argc > 2 ? argv[2] : dir_snt + ".out";

	Sora::MakeDirectory(dir_out);
	if (dir_out.length() > 0 && dir_out.back() != '\\') dir_out.push_back('\\');

	dir_snt.push_back('\\');
	vector<string> fn_snts;
	Sora::SearchFiles(dir_snt + "*" ATTR_SNT, fn_snts);

	const string TextBeg = "'";
	const string TextEnd = "\"";

	ofstream ofs_rp(REP_NAME);
	for (const auto &fn_snt : fn_snts) {
		const string name = fn_snt.substr(0, fn_snt.rfind(ATTR_SNT));
		cout << "处理" << fn_snt << "..." << endl;

		ifstream ifs(dir_snt + fn_snt);
		SoraSNT snt(ifs);
		ifs.close();

		ofstream ofs;
		int out_cnt = 0;
		int talk_cnt = 0;
		for(int i = 0; i < snt.Num(); i++) {
			const auto& item = snt[i];
			if(item.Type == AllItemTypes::Nomarl) continue;

			if(out_cnt == 0) ofs.open(dir_out + fn_snt);

			talk_cnt++;
			ofs << Seperator << "\n" << Seperator << "\n\n";

			assert(item.Num() >= item.Type->TextStartLine + 3);
			assert(item.Lines[item.Type->TextStartLine].content == TextBeg);
			assert(item.Lines.back().content == TextEnd);
			for(auto j = item.Type->TextStartLine + 1; j < item.Lines.size() - 1; j++) {
				out_cnt++;

				ofs << item.Type->Mark
					<< setfill('0') << setw(4) << setiosflags(ios::right) << talk_cnt << ","
					<< setfill('0') << setw(2) << setiosflags(ios::right) << j << ","
					<< item[j].content
					<< "\n\n";
			}
		}
		if(out_cnt > 0) ofs.close();

		ofs_rp << name << '\t' <<  out_cnt << endl;
	}
	ofs_rp.close();

	return 0;
}

#include <iostream>
#include <fstream>
#include <iomanip>

#include "common.h"

#define ATTR_SNT "._SN.txt"
#define ATTR_OUT ".txt"
#define REP_NAME "snt_report.txt"

#define MAXCH_ONELINE 2048

#define FLAG_CLEAR 0
#define FLAG_SAY 1
#define FLAG_TEXT 2
#define FLAG_TALK 4
#define FLAG_MENU 8


#define STR_SAY "say"
#define STR_TEXT "text"
#define STR_TALK "talk"
#define STR_MENU "menu"
#define STR_4TBL "\t\t\t\t"
#define CH_TBL '\t'
#define CH_SQ '\''

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

	ofstream ofs_rp(REP_NAME);
	for (const auto &fn_snt : fn_snts) {
		const string name = fn_snt.substr(0, fn_snt.rfind(ATTR_SNT));
		cout << "´¦Àí" << fn_snt << "..." << endl;

		ifstream ifs(dir_snt + fn_snt);
		ofstream ofs;
		int out_cnt = 0;
		
		char buff_snt[MAXCH_ONELINE + 1];
		int line_cnt = 0;

		int flag = FLAG_CLEAR;
		int line_no = 0;
		string text;

		while (ifs.getline(buff_snt, sizeof(buff_snt)).good())
		{
			line_cnt++;
			string line = buff_snt;
			
			char ch = line[0];
			if (line.find(STR_SAY) == 0 || (ch == '\"' &&  line.find(STR_SAY) == 1)) {
				flag = FLAG_SAY;
			}
			else if (line.find(STR_TEXT) == 0 || (ch == '\"' &&  line.find(STR_TEXT) == 1)) {
				flag = FLAG_TEXT;
			}
			else if (line.find(STR_TALK) == 0 || (ch == '\"' &&  line.find(STR_TALK) == 1)) {
				flag = FLAG_TALK;
			}
			else if (line.find(STR_MENU) == 0 || (ch == '\"' &&  line.find(STR_MENU) == 1)) {
				flag = FLAG_MENU;
			}
			else if(line.find(STR_4TBL) == 0 && (FLAG_SAY == flag || FLAG_TEXT == flag || FLAG_TALK == flag)) {
				if (text.empty()) {
					line_no = line_cnt;
				}
				text += line.c_str() + 4;

				/*if (text.length() >= 2 && text[text.length() - 2] == '\\' && text[text.length() - 1] == '3') {
					text.pop_back();
					text.pop_back();
				}

				if (text.length() >= 2 && text[text.length() - 2] == '\\' && text[text.length() - 1] == '2') {
					text.pop_back();
					text.pop_back();

					if (out_cnt == 0) {
						ofs.open(dir_out + name + ATTR_OUT);
					}
					ofs << setfill('0') << setw(6) << setiosflags(ios::right) << line_no << ","
						<< text << "\n\n";
					++out_cnt;
					text.clear();
				}
				else if (text.length() >= 2 && text[text.length() - 2] == '\\' && text[text.length() - 1] == '1') {
					text.back() = 'n';
				}*/
				//if ((text.length() >= 2 && text[text.length() - 2] == '\\' && text[text.length() - 1] == '2') || (text.length() >= 4 && text[text.length() - 4] == '\\' && text[text.length() - 3] == '2')) {
				if (text.find("\\2") != string::npos) {
					if (out_cnt == 0) {
						ofs.open(dir_out + name + ATTR_OUT);
					}
					ofs << setfill('0') << setw(6) << setiosflags(ios::right) << line_no << ","
						<< text << "\n\n";
					++out_cnt;
					text.clear();
				}
				else {
					text.append("\\n");
				}
			}
			else if (line.find(CH_SQ) == 0 && FLAG_TALK == flag) {
				;
			}
			else {
				flag = FLAG_CLEAR;
			}
		}

		ifs.close();
		if(out_cnt > 0) ofs.close();

		ofs_rp << name << '\t' <<  out_cnt << endl;
	}
	ofs_rp.close();

	return 0;
}

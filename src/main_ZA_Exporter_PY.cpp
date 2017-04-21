#include <iostream>
#include <fstream>
#include <iomanip>

#include "common.h"

#define ATTR_PY ".py"
#define ATTR_OUT ".txt"
#define REP_NAME "py_report.txt"

#define MAXCH_ONELINE 2048

constexpr char AnonymousTalk[] = "AnonymousTalk";
constexpr char ChrTalk[] = "ChrTalk";
constexpr char NpcTalk[] = "NpcTalk";

constexpr char Seperator[] = "#-------------------------------------------------------------------#";

using namespace std;

const string Talks[] = { AnonymousTalk, ChrTalk, NpcTalk };

int main(int argc, char* argv[])
{
	if (argc <= 1) {
		cout << "Usage:\n"
			"\t" "ZA_Exporter_PY dir_py [dir_out]\n"
			"\n"
			"Default: dir_out = dir_py.out"
			<< endl;
		return 0;
	}

	string dir_py = argv[1];  while (dir_py.back() == '/' || dir_py.back() == '\\') dir_py.pop_back();
	string dir_out = argc > 2 ? argv[2] : dir_py + ".out";

	Sora::MakeDirectory(dir_out);
	if (dir_out.length() > 0 && dir_out.back() != '\\') dir_out.push_back('\\');

	dir_py.push_back('\\');
	vector<string> fn_pys;
	Sora::SearchFiles(dir_py + "*" ATTR_PY, fn_pys);

	ofstream ofs_rp(REP_NAME);
	for (const auto &fn_py : fn_pys) {
		const string name = fn_py.substr(0, fn_py.rfind(ATTR_PY));
		cout << "处理" << fn_py << "..." << endl;

		ifstream ifs(dir_py + fn_py);
		ofstream ofs;
		int out_cnt = 0;
		int msg_cnt = 0;
		
		char buff[MAXCH_ONELINE + 1];
		string talk;
		int bra_cnt = 0;
		int cnt = 0;

		int line_no = 0;
		while (ifs.getline(buff, sizeof(buff)))
		{
			line_no++;
			string s = buff;
			if (!talk.empty()) {
				cnt++;
				string sout;

				size_t idx = s.find_first_of("\"'");
				while (idx != string::npos)
				{
					size_t idx_next = s.find_first_of("\"'", idx + 1);
					if (idx_next != string::npos) {
						sout += s.substr(idx + 1, idx_next - idx - 1);
					}
					idx = idx_next;
				}

				if (!sout.empty()) {
					if (out_cnt == 0) {
						ofs.open(dir_out + name + ATTR_OUT);
					}

					out_cnt++;
					ofs << talk[0]
						<< setfill('0') << setw(3) << setiosflags(ios::right) << msg_cnt << ","
						<< setfill('0') << setw(2) << setiosflags(ios::right) << cnt << ","
						<< setfill('0') << setw(5) << setiosflags(ios::right) << line_no << ","
						<< sout << "\n\n";
				}

				for (char c : s) {
					if (c == '(') ++bra_cnt;
					else if (c == ')') --bra_cnt;
				}
				if (bra_cnt <= 0) {
					talk.clear();

					ofs << Seperator << "\n\n";
				}
			} //if (talk) 
			else {
				for (const auto& search : Talks) {
					if (s.find(search) != string::npos) {
						bra_cnt = 1;
						cnt = 0;
						++msg_cnt;
						talk = search;
					}
				}
			}
		}

		ifs.close();
		if(out_cnt > 0) ofs.close();

		ofs_rp << name << '\t' <<  out_cnt << endl;
	}
	ofs_rp.close();

	return 0;
}

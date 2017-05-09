#include <iostream>
#include <fstream>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <cstdarg>

#include "Common.h"

using namespace std;

#define ATTR_PY ".bin.py"
#define ATTR_OUT ".txt"
#define REP_NAME "import_py_report.txt"

#define MAXCH_ONELINE 100000
#define MAX_LINENO_LEN 9

#define MAX_LINENO 0x7FFFFFFF

const string mark_talk_id = "#talk#";
const string mark_line_id = "#line#";
const string mark_fake = "#fake";

constexpr char AnonymousTalk[] = "AnonymousTalk";
constexpr char ChrTalk[] = "ChrTalk";
constexpr char NpcTalk[] = "NpcTalk";

const string TalkTypes[] = { AnonymousTalk, ChrTalk, NpcTalk };

const string SPACE = "            ";

static int cnt_err = 0;
static ofstream ofs_rp;
static void Error(const char* foramt, ...) {
	if (cnt_err == 0) ofs_rp.open(REP_NAME);

	cnt_err++;
	static char buff[MAXCH_ONELINE];
	va_list argptr;
	va_start(argptr, foramt);
	vsnprintf(buff, sizeof(buff), foramt, argptr);
	va_end(argptr);

	ofs_rp << buff << endl;
}

struct LineInfo
{
	string vid;
	int line_no;
	string ex1;
	string ex2;
};
using LinesInTalk = unordered_map<int, LineInfo>;
using Talks = unordered_map<int, LinesInTalk>;

static auto GetMapTalkVid(const string& py_out, const string& bin_out) {
	Talks rst;

	ifstream ifs_py(py_out);
	ifstream ifs_bin(bin_out);

	if (!ifs_py || !ifs_bin) return rst;

	static char buff_py[MAXCH_ONELINE + 1];
	static char buff_bin[MAXCH_ONELINE + 1];

	for (int line_no = 0;
		ifs_py.getline(buff_py, sizeof(buff_py)) && ifs_bin.getline(buff_bin, sizeof(buff_bin));
		line_no++ ) {

		bool empty_py = buff_py[0] == '#' || buff_py[0] == '\0' || buff_py[0] == '\t';
		bool empty_bin = buff_bin[0] == '#' || buff_bin[0] == '\0' || buff_bin[0] == '\t';

		if (empty_py != empty_bin) {
			Error("%s, %d: 空行不匹配！", py_out.c_str(), line_no);
		}
		if (empty_py || empty_bin) continue;

		int talk_id = -1, line_id = -1, ori_line_no = -1;
		char type = 0;
		if (sscanf(buff_py, "%c%04d,%02d,%05d,", &type, &talk_id, &line_id, &ori_line_no) < 4) {
			Error("%s, %d: 错误的行！", py_out.c_str(), line_no);
			continue;
		}

		int pos = 0;
		string vid;
		while (vid.empty() && buff_bin[pos])
		{
			while (buff_bin[pos] && buff_bin[pos] != '#') pos++;
			if (buff_bin[pos] == '#') {
				int start = pos;
				pos++;
				while (buff_bin[pos] >= '0' && buff_bin[pos] <= '9') pos++;
				if (buff_bin[pos] == 'V' && pos - start > 1) {
					vid.assign(buff_bin + start, pos - start);
					vid.push_back('v');
					break;
				}
			}
		}

		string ex1("## ");
		pos = 0;
		while (buff_bin[pos])
		{
			while (buff_bin[pos] && buff_bin[pos] != '#') pos++;
			if (buff_bin[pos] == '#') {
				int start = pos;
				pos++;
				while (buff_bin[pos] >= '0' && buff_bin[pos] <= '9') pos++;
				if ((buff_bin[pos] == 'A' || buff_bin[pos] == 'W') && pos - start > 1) {
					ex1.append(buff_bin + start, pos - start + 1);
				}
			}
		}
		if (ex1.length() <= 3) ex1.clear();

		string ex2;
		for (pos = 0; buff_py[pos + 1]; pos++) {
			if (buff_py[pos] == '#' && buff_py[pos + 1] == '#') {
				ex2.append(buff_py + pos).append(" ");
			}
		}
		for (pos = 0; buff_bin[pos + 1]; pos++) {
			if (buff_bin[pos] == '#' && buff_bin[pos + 1] == '#') {
				ex2.append(buff_bin + pos).append(" ");
			}
		}


		if (!vid.empty() || !ex2.empty() || !ex1.empty()) {
			auto &Talk = rst[talk_id];
			auto inrst = Talk.insert({ line_id, { vid, line_no, ex1, ex2 } });

			if (!inrst.second) {
				Error("%s, %d: %04d,%02d,%05d, 重复的键值！", py_out.c_str(), line_no, talk_id, line_id, ori_line_no);
			}
		}
	}

	return rst;
}

int main(int argc, char* argv[])
{

	if (argc <= 4) {
		cout << "Usage:\n"
			"\t" "ZA_Importer_PY dir_py_new dir_py_old dir_py_out dir_bin_out\n"
			<< endl;
		return 0;
	}

	string dir_py_new = argv[1];
	string dir_py = argv[2];
	string dir_py_out = argv[3];
	string dir_bin_out = argv[4];

	Sora::MakeDirectory(dir_py_new);
	if (dir_py_new.length() > 0 && dir_py_new.back() != '\\') dir_py_new.push_back('\\');
	if (dir_py.length() > 0 && dir_py.back() != '\\') dir_py.push_back('\\');
	if (dir_py_out.length() > 0 && dir_py_out.back() != '\\') dir_py_out.push_back('\\');
	if (dir_bin_out.length() > 0 && dir_bin_out.back() != '\\') dir_bin_out.push_back('\\');

	vector<string> fn_pys;
	Sora::SearchFiles(dir_py + "*" ATTR_PY, fn_pys);

	for (const auto &fn_py : fn_pys) {
		string name = fn_py.substr(0, fn_py.rfind(ATTR_PY));
		cout << "处理" << fn_py << "..." << endl;

		const auto talks = GetMapTalkVid(dir_py_out + name + ATTR_OUT, dir_bin_out + name + ATTR_OUT);

		ifstream ifs(dir_py + fn_py);
		ofstream ofs(dir_py_new + fn_py);

		char buff[MAXCH_ONELINE + 1];

		string talk_type;
		int bra_cnt = 0;

		int talk_id_cnt = 0;
		int line_id_cnt = 0;
		auto it_talk = talks.end();

		int talk_id = talk_id_cnt;
		int line_id = line_id_cnt;

		for (int line_no = 1;
			ifs.getline(buff, sizeof(buff));
			line_no++)
		{
			string s = buff;

			if (!talk_type.empty()) {
				if (s.find('"') == string::npos) {
					for (char c : s) {
						if (c == '(') ++bra_cnt;
						else if (c == ')') --bra_cnt;
					}
				}

				if (bra_cnt <= 0) {
					talk_type.clear();
				}

				if (it_talk != talks.end()) {
					auto index = s.find(mark_line_id);
					if (index != string::npos) {
						if (0 == sscanf(s.c_str() + index + mark_line_id.length(), "%d", &line_id)) {
							line_id = -1;
							Error("%s, %04d: 无效的line_id！", name.c_str(), line_no);
						}
					}
					else {
						++line_id_cnt;
						line_id = line_id_cnt;
					}

					auto it_line = it_talk->second.find(line_id);

					if (it_line != it_talk->second.end()) {
						auto idx = s.find('"');
						if (idx == string::npos) {
							Error("%s, %04d,%02d,%05d: 该行无文本！", name.c_str(), line_no, talk_id_cnt, line_id_cnt);
						}
						else {
							s.insert(idx + 1, it_line->second.vid);
							if (!it_line->second.ex1.empty() || !it_line->second.ex2.empty()) {
								s.append(SPACE).append(it_line->second.ex1).append(it_line->second.ex2);
							}
						}
					}
				}
			} //if (talk) 
			else {
				if (s.find(mark_fake) != string::npos) {
					++talk_id_cnt;
				}
				for (const auto& search : TalkTypes) {
					if (s.find(search) != string::npos) {
						bra_cnt = 1;
						line_id_cnt = 0;
						talk_type = search;

						auto index = s.find(mark_talk_id);
						if (index != string::npos) {
							if (0 == sscanf(s.c_str() + index + mark_talk_id.length(), "%d", &talk_id)) {
								talk_id = -1;
								Error("%s, %04d: 无效的talk_id！", name.c_str(), line_no);
							}
						}
						else {
							++talk_id_cnt;
							talk_id = talk_id_cnt;
						}

						it_talk = talks.find(talk_id);
					}
				}
			}

			ofs << s << '\n';
		}

		ifs.close();
		ofs.close();
	}

	if (cnt_err > 0) {
		ofs_rp.close();
		cout << "存在错误，参阅报告：" << REP_NAME << endl;
		getchar();
	}

	return 0;
}

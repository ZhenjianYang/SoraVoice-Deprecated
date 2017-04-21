#include <iostream>
#include <fstream>
#include <map>
#include <tuple>
#include <algorithm>
#include <cstdarg>

#include "common.h"

using namespace std;

#define ATTR_PY ".bin.py"
#define ATTR_OUT ".txt"
#define REP_NAME "import_py_report.txt"

#define MAXCH_ONELINE 100000
#define MAX_LINENO_LEN 9

#define MAX_LINENO 0x7FFFFFFF

constexpr char AnonymousTalk[] = "AnonymousTalk";
constexpr char ChrTalk[] = "ChrTalk";
constexpr char NpcTalk[] = "NpcTalk";

const string Talks[] = { AnonymousTalk, ChrTalk, NpcTalk };

static int cnt_err = 0;
static ofstream ofs;
static void Error(const char* foramt, ...) {
	if (cnt_err == 0) ofs.open(REP_NAME);

	cnt_err++;
	static char buff[MAXCH_ONELINE];
	va_list argptr;
	va_start(argptr, foramt);
	vsnprintf(buff, sizeof(buff), foramt, argptr);
	va_end(argptr);

	ofs << buff << endl;
}

static auto GetMapVid(const string& py_out, const string& bin_out, bool lineNoMode = false) {
	map<int, tuple<int, int, int, string>> map_vid{ { MAX_LINENO,{} } };

	ifstream ifs_py(py_out);
	ifstream ifs_bin(bin_out);

	if (!ifs_py || !ifs_bin) return map_vid;

	static char buff_py[MAXCH_ONELINE + 1];
	static char buff_bin[MAXCH_ONELINE + 1];

	for (int i = 0;
		ifs_py.getline(buff_py, sizeof(buff_py)) && ifs_bin.getline(buff_bin, sizeof(buff_bin));
		i++ ) {

		if (buff_py[0] == '#' && buff_bin[0] != '#' || buff_py[0] != '#' && buff_bin[0] == '#') {
			Error("%s, %d: 注释行不匹配！", py_out.c_str(), i);
		}
		if (buff_py[0] == '#' || buff_bin[0] == '#') continue;

		if (buff_py[0] == '\0' && buff_bin[0] != '\0' || buff_py[0] != '\0' && buff_bin[0] == '\0') {
			Error("%s, %d: 空行不匹配！", py_out.c_str(), i);
		}
		if (buff_py[0] == '\0' || buff_bin[0] == '\0') continue;

		int msg_cnt = -1, cnt = -1, line_no = -1;
		char type = 0;
		if (sscanf(buff_py, "%c%04d,%02d,%05d,", &type, &msg_cnt, &cnt, &line_no) < 4) {
			Error("%s, %d: 错误的行！", py_out.c_str(), i);
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
					vid.assign(buff_bin + start, pos - start + 1);
					break;
				}
			}
		}
		if (!vid.empty()) {
			int key = lineNoMode ? i : msg_cnt * 100 + cnt;
			auto inrst = map_vid.insert({ key, { msg_cnt, cnt, line_no, vid } });
			if (!inrst.second) {
				Error("%s, %d: %04d,%02d,%05d, 重复的键值！", py_out.c_str(), i, msg_cnt, cnt, line_no);
			}
		}
	}

	return map_vid;
}

int main(int argc, char* argv[])
{
	bool LineNoMode = false;

	int argi = 1;
	bool enbaleMappingGlobal = false;
	if (argc > 1 && argv[argi][0] == '-') {
		if (argv[argi][1] == 'm') {
			enbaleMappingGlobal = true;
			++argi;
		}
	}

	if (argc - argi < 4) {
		cout << "Usage:\n"
			"\t" "ZA_Importer_PY [-l] dir_py_new dir_py_old dir_py_out dir_bin_out\n"
			"\t\t" "-l : line no mode"
			<< endl;
		return 0;
	}

	int iarg = 1;
	if (argv[iarg][0] == '-') {
		if (argv[iarg][1] == 'l') { LineNoMode = true; iarg++; }
	}

	string dir_py_new = argv[iarg];
	string dir_py = argv[iarg + 1];
	string dir_py_out = argv[iarg + 2];
	string dir_bin_out = argv[iarg + 3];

	Sora::MakeDirectory(dir_py_new);
	if (dir_py_new.length() > 0 && dir_py_new.back() != '\\') dir_py_new.push_back('\\');
	if (dir_py.length() > 0 && dir_py.back() != '\\') dir_py.push_back('\\');
	if (dir_py_out.length() > 0 && dir_py_out.back() != '\\') dir_py_out.push_back('\\');
	if (dir_bin_out.length() > 0 && dir_bin_out.back() != '\\') dir_bin_out.push_back('\\');

	vector<string> fn_pys;
	Sora::SearchFiles(dir_py + "*" ATTR_PY, fn_pys);

	ofstream ofs_rp(REP_NAME);
	for (const auto &fn_py : fn_pys) {
		string name = fn_py.substr(0, fn_py.rfind(ATTR_PY));
		cout << "处理" << fn_py << "..." << endl;

		const auto map_vid = GetMapVid(dir_py_out + name + ATTR_OUT, dir_bin_out + name + ATTR_OUT, LineNoMode);

		ifstream ifs(dir_py + fn_py);
		ofstream ofs(dir_py_new + fn_py);

		char buff[MAXCH_ONELINE + 1];

		string talk;
		int bra_cnt = 0;

		int msg_cnt = 0;
		int cnt = 0;
		int line_no = 0;

		int key = 0;
		auto it = map_vid.cbegin();

		for (int line_no = 1;
			ifs.getline(buff, sizeof(buff));
			line_no++)
		{
			string s = buff;

			if (!talk.empty()) {
				cnt++;

				if (s.find('"') == string::npos) {
					for (char c : s) {
						if (c == '(') ++bra_cnt;
						else if (c == ')') --bra_cnt;
					}
				}

				if (bra_cnt <= 0) {
					talk.clear();
				}
				
				key = LineNoMode ? line_no : msg_cnt * 100 + cnt;

				while (key > it->first)
				{
					Error("%s, %04d,%02d,%05d,%s: 未找到插入位置！", get<0>(it->second), get<1>(it->second), get<2>(it->second), get<3>(it->second).c_str());
					++it;
				}

				if (key == it->first) {
					auto idx = s.find('"');
					if (idx == string::npos) {
						Error("%s, %04d,%02d,%05d: 该行无文本！", name.c_str(), msg_cnt, cnt);
					}
					else {
						s = s.insert(idx + 1, get<3>(it->second));
					}
					++it;
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

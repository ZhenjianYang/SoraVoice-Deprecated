#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <unordered_map>
#include <algorithm>

#include <cstdarg>

#include "Common.h"
#include "Mapping.h"
#include "SoraIE.h"

#define ATTR_SNT "._SN.txt"
#define ATTR_IPT ".txt"
#define REP_NAME "import_snt_report.txt"

#define MAXCH_ONELINE 10000

using namespace std;

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

struct LineInfo
{
	string vid;
	int line_no;
	string ex1;
};
using LinesInTalk = unordered_map<int, LineInfo>;
using Talks = unordered_map<int, LinesInTalk>;

static auto GetMapTalkVid(const string& snt_out, const string& bin_out) {
	Talks rst;

	ifstream ifs_snt(snt_out);
	ifstream ifs_bin(bin_out);

	if (!ifs_snt || !ifs_bin) return rst;

	static char buff_snt[MAXCH_ONELINE + 1];
	static char buff_bin[MAXCH_ONELINE + 1];

	for (int line_no = 1;
		ifs_snt.getline(buff_snt, sizeof(buff_snt)) && ifs_bin.getline(buff_bin, sizeof(buff_bin));
		line_no++) {

		bool empty_py = buff_snt[0] == '#' || buff_snt[0] == '\0';
		bool empty_bin = buff_bin[0] == '#' || buff_bin[0] == '\0';

		if (empty_py != empty_bin) {
			Error("%s, %d: 空行不匹配！", snt_out.c_str(), line_no);
		}
		if (empty_py || empty_bin) continue;

		int talk_id = -1, line_id = -1;
		char type = 0;
		if (sscanf(buff_snt, "%c%04d,%02d,", &type, &talk_id, &line_id) < 3) {
			Error("%s, %d: 错误的行！", snt_out.c_str(), line_no);
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

		string ex1;
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

		if (!vid.empty() || !ex1.empty()) {
			auto &Talk = rst[talk_id];
			auto inrst = Talk.insert({ line_id,{ vid, line_no, ex1} });

			if (!inrst.second) {
				Error("%s, %d: %04d,%02d,%05d, 重复的键值！", snt_out.c_str(), line_no, talk_id, line_id);
			}
		}
	}

	return rst;
}

int main(int argc, char* argv[])
{
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
			"\t" "Importer_SNT.exe [-m] dir_snt_new dir_snt_old dir_snt_exp dir_mbin_exp [exception_list]\n"
			"\t\t" "-m             : Set the opinion \"Voice ID Mapping\" to Enable. (Default is Disbale)\n"
			"\t\t" "exception_list : This is a file contains an exception list. \n"
			"\t\t" "                 Files in this list will have an opposite setting of opinion \"Voice ID Mapping\"\n"
			<< endl;
		return 0;
	}

	string dir_out = argv[argi + 0];
	string dir_snt = argv[argi + 1];
	string dir_snt_exp = argv[argi + 2];
	string dir_mbin_exp = argv[argi + 3];

	set<string> exception_list;
	if (argc - argi >= 5) {
		ifstream ifs_excp(argv[argi + 4]);
		string s;
		while (ifs_excp >> s) {
			for_each(s.begin(), s.end(), [](char& c) { c = toupper(c); });
			exception_list.insert(s);
		}
		ifs_excp.close();
	}

	unordered_map<string, string> map_vid;
#define BUFF_LEN 7
	constexpr int buff_len = BUFF_LEN;
	static_assert(buff_len >= MAX_VOICEID_LEN_NEED_MAPPING + 1, "buff_len not enougt");
	char buff_vid[buff_len + 1];

	for (int vid_len = MAX_VOICEID_LEN_NEED_MAPPING; vid_len > 0; vid_len--) {
		for (int i = VoiceIdAdjustAdder[vid_len]; i < VoiceIdAdjustAdder[vid_len - 1] && i < NUM_MAPPING; i++) {
			if (VoiceIdMapping[i][0]) {
				sprintf(buff_vid, "%07d", i - VoiceIdAdjustAdder[vid_len]);
				string vid_ori = '#' + string(VoiceIdMapping[i]) + 'v';
				string vid_mapped = '#' + string(buff_vid + buff_len - vid_len) + 'v';
				auto rst = map_vid.insert({ vid_ori, vid_mapped } );
				if (!rst.second) {
					cout << "Duplicate voice ID: " << VoiceIdMapping[i] << endl;
				}
			}
		}
	}

	Sora::MakeDirectory(dir_out);
	if (dir_out.length() > 0 && dir_out.back() != '\\') dir_out.push_back('\\');
	if (dir_snt.length() > 0 && dir_snt.back() != '\\') dir_snt.push_back('\\');
	if (dir_snt_exp.length() > 0 && dir_snt_exp.back() != '\\') dir_snt_exp.push_back('\\');
	if (dir_mbin_exp.length() > 0 && dir_mbin_exp.back() != '\\') dir_mbin_exp.push_back('\\');

	vector<string> fn_snts;
	Sora::SearchFiles(dir_snt + "*" ATTR_SNT, fn_snts);

	ofstream ofs_rp(REP_NAME);
	for (const auto &fn_snt : fn_snts) {
		string name = fn_snt.substr(0, fn_snt.rfind(ATTR_SNT));
		for_each(name.begin(), name.end(), [](char& c) { c = toupper(c); });
		ofs_rp << "处理" << fn_snt << "..." << endl;
		cout << "处理" << fn_snt << "..." << endl;

		bool enbaleMapping = enbaleMappingGlobal && exception_list.find(name) == exception_list.end()
			|| !enbaleMappingGlobal && exception_list.find(name) != exception_list.end();

		const auto map_talk_vid = GetMapTalkVid(dir_snt_exp + name + ATTR_IPT, dir_mbin_exp + name + ATTR_IPT);

		ifstream ifs(dir_snt + fn_snt);
		SoraSNT snt(ifs);
		ifs.close();

		int cnt_talk = 0;
		for (int i = 0; i < snt.Num(); i++) {
			auto &item = snt[i];

			if (item.Type == AllItemTypes::Nomarl) continue;

			cnt_talk++;
			auto it_talk = map_talk_vid.find(cnt_talk);
			if (it_talk == map_talk_vid.end()) continue;

			for (auto &line : it_talk->second) {
				if (line.second.vid.empty()) continue;

				if (line.first <= (int)item.Type->TextStartLine || line.first >= (int)item.Num() - 1) {
					Error("%s, %c%04d, %02d: 无法在此处插入语音 %s (来自行%d)！",
						name.c_str(), item.Type->Mark, cnt_talk, line.first,
						line.second.vid.c_str(), line.second.vid);
				}
				else {
					auto it = map_vid.end();

					if (enbaleMapping) {
						it = map_vid.find(line.second.vid);
					}

					if (it == map_vid.end()) {
						item[line.first].content = line.second.vid + item[line.first].content;
					}
					else {
						item[line.first].content = it->second + item[line.first].content;
					}
				} //else
			}//for (auto &line : it_talk->second)
		}

		ofstream ofs(dir_out + fn_snt);
		snt.Output(ofs);
		ofs.close();
	}

	if (cnt_err > 0) {
		ofs_rp.close();
		cout << "存在错误，参阅报告：" << REP_NAME << endl;
		getchar();
	}
	return 0;
}

#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <unordered_map>
#include <algorithm>

#include "common.h"
#include "mapping.h"

#define ATTR_SNT "._SN.txt"
#define ATTR_IPT ".txt"
#define REP_NAME "import_snt_report.txt"

#define MAXCH_ONELINE 2048
#define MAX_LINENO_LEN 9

#define MAX_LINENO 0x7FFFFFFF

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

static void GetMapLineVid(map<int, string>& map_line_vid, const string& fn_snt_exp, const string& fn_mbin_exp, ofstream& ofs_rp)
{
	ifstream ifs_snt(fn_snt_exp);
	ifstream ifs_mbin(fn_mbin_exp);

	if (!ifs_snt || !ifs_mbin) return;

	char buf_snt[MAXCH_ONELINE + 1], buf_mbin[MAXCH_ONELINE + 1];
	while (ifs_snt.getline(buf_snt, sizeof(buf_snt)).good()
		&& ifs_mbin.getline(buf_mbin, sizeof(buf_mbin)).good())
	{
		int line_no = 0;
		for (int i = 0; i < MAX_LINENO_LEN; i++) {
			if (buf_snt[i] == ',') break;
			else if (buf_snt[i] >= '0' && buf_snt[i] <= '9') {
				line_no = line_no * 10 + buf_snt[i] - '0';
			}
			else {
				line_no = 0;
				break;
			}
		}
		if (line_no <= 0) continue;

		int pos = 0;
		string vid;
		while (vid.empty() && buf_mbin[pos])
		{
			while (buf_mbin[pos] && buf_mbin[pos] != '#') pos++;
			if (buf_mbin[pos] == '#') {
				int start = pos;
				pos++;
				while (buf_mbin[pos] >= '0' && buf_mbin[pos] <= '9') pos++;
				if (buf_mbin[pos] == 'V' && pos - start > 1) {
					vid.assign(buf_mbin + start + 1, pos - start - 1);
					break;
				}
			}
		}
		if (map_line_vid[line_no].empty()) {
			if (!vid.empty()) map_line_vid[line_no] = vid;
		}
		else {
			ofs_rp << ">>【警告】行 " << line_no << " ：在SNT.OUT中发现重复的该行号。" << endl;
		}

	}

	ifs_snt.close();
	ifs_mbin.close();
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

	map<string, string> map_vid;
#define BUFF_LEN 7
	constexpr int buff_len = BUFF_LEN;
	static_assert(buff_len >= MAX_VOICEID_LEN_NEED_MAPPING + 1, "buff_len not enougt");
	char buff_vid[buff_len + 1];

	for (int vid_len = MAX_VOICEID_LEN_NEED_MAPPING; vid_len > 0; vid_len--) {
		for (int i = VoiceIdAdjustAdder[vid_len]; i < VoiceIdAdjustAdder[vid_len - 1] && i < NUM_MAPPING; i++) {
			if (VoiceIdMapping[i][0]) {
				sprintf(buff_vid, "%07d", i - VoiceIdAdjustAdder[vid_len]);
				auto rst = map_vid.insert({ VoiceIdMapping[i], buff_vid + buff_len - vid_len });
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

		bool enbaleMapping = enbaleMappingGlobal && exception_list.find(name) == exception_list.end()
			|| !enbaleMappingGlobal && exception_list.find(name) != exception_list.end();

		map<int, string> map_line_vid;
		GetMapLineVid(map_line_vid, dir_snt_exp + name + ATTR_IPT, dir_mbin_exp + name + ATTR_IPT, ofs_rp);
		map_line_vid.insert({ MAX_LINENO,"" });

		auto it_mlv = map_line_vid.cbegin();

		ifstream ifs(dir_snt + fn_snt);
		ofstream ofs(dir_out + fn_snt);

		char buff[MAXCH_ONELINE + 1];
		for (int line_no = 1;
			ifs.getline(buff, sizeof(buff)).good();
			line_no++)
		{
			if (line_no == it_mlv->first) {
				if (buff[0] == '\t' && buff[1] == '\t' && buff[2] == '\t' && buff[3] == '\t') {
					auto it_mapping = map_vid.cend();
					if (enbaleMapping) {
						it_mapping = map_vid.find(it_mlv->second);
					}

					if (it_mapping != map_vid.cend()) {
						ofs << STR_4TBL << '#' << it_mapping->second << 'V' << buff + 4 << '\n';
					}
					else {
						ofs << STR_4TBL << '#' << it_mlv->second << 'V' << buff + 4 << '\n';
					}

				}
				else {
					ofs_rp << ">>【警告】行 " << line_no << " ：在SNT的该行没有发现有效文本。" << endl;
					ofs << buff << '\n';
				}
				it_mlv++;
			}
			else {
				ofs << buff << '\n';
			}
		}

		ifs.close();
		ofs.close();
	}
	ofs_rp.close();

	return 0;
}

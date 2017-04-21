#include <iostream>
#include <fstream>
#include <iomanip>
#include <memory>
#include <map>
#include <algorithm>

#include "common.h"

#define ATTR_BIN ".bin"
#define ATTR_OUT ".txt"
#define REP_NAME "bin_report.txt"

#define GET_INT(ptr) *(int*)(ptr)
#define GET_U32(ptr) *(unsigned*)(ptr)
#define GET_U16(ptr) *(unsigned short*)(ptr)

#define OFF_OFF_STRING 0x34
#define OFF_OFF_FUN_TABLE 0x42

using namespace std;

using byte = unsigned char;
using u8 = byte;
using u16 = unsigned short;

constexpr char AnonymousTalk[] = "AnonymousTalk";
constexpr char ChrTalk[] = "ChrTalk";
constexpr char NpcTalk[] = "NpcTalk";
constexpr char Invalid[] = "";
constexpr byte CODE_AnonymousTalk = 0x50;//AO 0x52
constexpr byte CODE_ChrTalk = 0x57;//AO 0x59
constexpr byte CODE_NpcTalk = 0x58;//AO 0x5A
constexpr byte CODE_Invalid = 0;

constexpr byte SCPSTR_CODE_LINE_FEED = 0x01;
constexpr byte SCPSTR_CODE_ENTER = 0x02;
constexpr byte SCPSTR_CODE_CLEAR = 0x03;
constexpr byte SCPSTR_CODE_05 = 0x05;
constexpr byte SCPSTR_CODE_COLOR = 0x07;
constexpr byte SCPSTR_CODE_09 = 0x09;
constexpr byte SCPSTR_CODE_ITEM = 0x1F;

static const map<byte, int> scp_str_list = {
		{0x01,0},     //SCPSTR_CODE_LINE_FEED
		{0x02,0},     //SCPSTR_CODE_ENTER
		{0x03,0},     //SCPSTR_CODE_CLEAR
		{0x04,0},
		{0x05,0},
		{0x06,0},     //SCPSTR_CODE_05
		{0x07,1},     //SCPSTR_CODE_COLOR
		{0x09,0},     //SCPSTR_CODE_09
		{0x0A,0},
		{0x0D,0},
		{0x18,0},
		{0x1F,2},     //SCPSTR_CODE_ITEM
};

constexpr const byte CODE_LIST[] = {CODE_AnonymousTalk, CODE_ChrTalk, CODE_NpcTalk};
constexpr const char* CODECH_LIST[] = {AnonymousTalk, ChrTalk, NpcTalk};
constexpr char Seperator[] = "#-------------------------------------------------------------------#";

static bool AoMode = false;
static char getCodeCh(byte code) {
	constexpr int len = std::extent<decltype(CODE_LIST)>::value;
	for(int i = 0; i < len; i++) {
		if(CODE_LIST[i] == code) return CODECH_LIST[i][0];
	}
	return Invalid[0];
}

static char getCode(byte raw_code) {
	constexpr int len = std::extent<decltype(CODE_LIST)>::value;
	if(AoMode) raw_code -= 2;
	for(int i = 0; i < len; i++) {
		if(CODE_LIST[i] == raw_code) return raw_code;
	}
	return CODE_Invalid;
}

class Talk {
public:
	const byte Code;
	const char CodeCh;

	bool Valid() const { return Code != CODE_Invalid && len; }
	int Length() const { return len; }
	const vector<pair<int, string>>& Texts() const { return texts; }
	const vector<pair<int, string>>& Names() const { return names; }
	const u16& Ch() const { return ch; }

	Talk(const byte* buf, int max_len)
		: Code(max_len > 5 ? getCode(buf[0]) : CODE_Invalid),
		  CodeCh(getCodeCh(Code)),
		  len(3), ch(GET_U16(buf + 1)) {
		if(Code == CODE_Invalid) return;

		if(Code == CODE_NpcTalk) {
			int len_names = parse(names, buf + len, max_len - len);
			if(len_names <= 0) { len = 0; return; };
			if(names.size() != 1) {
				len = 0; return;
			};
			for(int i = 0; i < (int)names[0].second.size(); i+= 2) {
				if((byte)names[0].second[i] < 0x80)  { len = 0; return; };
			}

			for_each(names.begin(), names.end(), [this](pair<int, string>& str) {
				str.first += len;
			});
			len += len_names;
		}

		int len_texts = parse(texts, buf + len, max_len - len);
		if(len_texts <= 0) { len = 0; return; };
		for_each(texts.begin(), texts.end(), [this](pair<int, string>& str) {
			str.first += len;
		});
		len += len_texts;

		if(texts.size() == 0 || texts.rbegin()->second.find("\\x02") == string::npos) {
			len = 0;
		}
	}

private:
	int len;
	u16 ch;
	vector<pair<int, string>> names;
	vector<pair<int, string>> texts;

	static int parse(vector<pair<int, string>>& strs, const byte* buf, const int max_len) {

		char numbuf[10];

		int len = 0;
		bool isStr = false;
		pair<int, string> tmp = {len, ""};

		while(len < max_len - 1) {
			if(buf[len] >= 0x20 && buf[len] < 0x7E) {
				if(!isStr && !tmp.second.empty()) {
					strs.push_back(tmp);
					tmp = {len, ""};
				}
				isStr = true;
				tmp.second.push_back(buf[len++]);
			}
			else if((buf[len] >= 0x81 && buf[len] <= 0x9F)
					|| (buf[len] >= 0xE0 && buf[len] <= 0xEF)) {
				if(len + 1 >= max_len) return 0;

				if(!isStr && !tmp.second.empty()) {
					strs.push_back(tmp);
					tmp = {len, ""};
				}
				isStr = true;

				if(buf[len+1] >= 0x40 && buf[len+1] <= 0xFC && buf[len+1] != 0x7F) {
					tmp.second.push_back(buf[len++]);
					tmp.second.push_back(buf[len++]);
				}
				else {
					return 0;
				}
			}
			else if(buf[len] == 0){
				if(tmp.second.size() == 1) return 0;
				break;
			}
			else {
				if(tmp.second.size() == 1) return 0;
				auto it = scp_str_list.find(buf[len]);
				if(it != scp_str_list.end()) {
					if(len + it->second + 1 > max_len) return 0;

					for(int i = 0; i < it->second + 1; i++) {
						sprintf(numbuf, "\\x%02X", buf[len++]);

						if(!tmp.second.empty() || isStr)
							tmp.second += numbuf;
					}
					isStr = false;
				}
				else
					return 0;
			}
		}

		if(!tmp.second.empty()) {
			strs.push_back(tmp);
		}

		return buf[len] == 0 && strs.size() > 0 ? len + 1 : 0;
	}
};

int main(int argc, char* argv[])
{
	if (argc <= 1) {
		cout << "Usage:\n"
			"\t" "ZA_Exporter_BIN [-a|z] dir_bin [dir_out]\n"
			"\t" "-a:  Ao no Kiseki\n"
			"\t" "-z:  Zero no Kiseki\n"
			"\n"
			"Default: dir_out = dir_bin.out"
			"         if dir_bin contains ao, then -a else -z"
			<< endl;
		return 0;
	}

	int iarg = 1;
	if(argv[iarg][0] == '-') {
		if(argv[iarg][1] == 'a') { AoMode = true; iarg++; }
		else if(argv[iarg][1] == 'z') { AoMode = false; iarg++; }
	}

	string dir_bin = argv[iarg];  while (dir_bin.back() == '/' || dir_bin.back() == '\\') dir_bin.pop_back();
	string dir_out = argc > iarg + 1 ? argv[iarg + 1] : dir_bin + ".out";

	if(iarg == 1) {
		for(char& c : dir_bin) c = tolower(c);
		AoMode = dir_bin.find("ao") != string::npos;
	}

	if(AoMode) {
		cout << "Ao no kiseki" << endl;
	}

	Sora::MakeDirectory(dir_out);
	if (dir_out.length() > 0 && dir_out.back() != '\\') dir_out.push_back('\\');

	dir_bin.push_back('\\');
	vector<string> fn_bins;
	Sora::SearchFiles(dir_bin + "*" ATTR_BIN, fn_bins);

	ofstream ofs_rp(REP_NAME);
	for (const auto &fn_bin : fn_bins) {
		const string name = fn_bin.substr(0, fn_bin.rfind(ATTR_BIN));
		cout << "处理" << fn_bin << "..." << endl;

		ifstream ifs(dir_bin + fn_bin, ios::binary);
		ofstream ofs;
		int out_cnt = 0;
		int msg_cnt = 0;
		
		ifs.seekg(0, ios::end);
		int len = (int)ifs.tellg();
		std::unique_ptr<byte[]> sbuff(new byte[len]);
		ifs.seekg(0, ios::beg);
		ifs.read((char*)sbuff.get(), len);
		ifs.close();
		const byte* const buff = sbuff.get();

		unsigned off_string = GET_U32(buff + OFF_OFF_STRING);
		unsigned off_fun_table = GET_U16(buff + OFF_OFF_FUN_TABLE);
		unsigned size_fun_table = GET_U16(buff + OFF_OFF_FUN_TABLE + 2);

		int fun_cnt = size_fun_table / 4;
		vector<int> funs;
		for (int i = 0; i < fun_cnt; i++) {
			funs.push_back(GET_INT(buff + off_fun_table + i * 4));
		}
		funs.push_back(off_string);

		for (int i = 0; i < fun_cnt; i++) {
			int start = funs[i];
			int end = funs[i+1];

			int idx = start;
			while(idx < end) {
				Talk talk(buff + idx, end - idx);
				if(!talk.Valid()) {
					++idx;
				}
				else {
					if (out_cnt == 0) {
						ofs.open(dir_out + name + ATTR_OUT);
					}
					msg_cnt++;
					int cnt = 0;

					auto output = [&](const vector<pair<int, string>>& strs) {
						for(auto& str : strs) {
							out_cnt++;
							cnt++;
							ofs << talk.CodeCh
								<< setfill('0') << setw(3) << setiosflags(ios::right) << dec << msg_cnt << ","
								<< setfill('0') << setw(2) << setiosflags(ios::right) << dec << cnt << ","
								<< setfill('0') << setw(5) << setiosflags(ios::right | ios::uppercase) << hex << idx + str.first << ","
								<< str.second << "\n\n";
						}
					};

					output(talk.Names());
					output(talk.Texts());
					ofs << Seperator << "\n\n";

					idx += talk.Length();
				}
			}
		}

		ofs_rp << name << '\t' <<  msg_cnt << '\t' << out_cnt << endl;
	}
	ofs_rp.close();

	return 0;
}

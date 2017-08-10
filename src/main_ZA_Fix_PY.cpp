#include <iostream>
#include <fstream>
#include <iomanip>
#include <unordered_map>
#include "Common.h"

#define ATTR_PY ".py"

#define MAXCH_ONELINE 100000

constexpr char AnonymousTalk[] = "AnonymousTalk";
constexpr char ChrTalk[] = "ChrTalk";
constexpr char NpcTalk[] = "NpcTalk";

using namespace std;

const string TalkTypes[] = { AnonymousTalk, ChrTalk, NpcTalk };
const string Str_Sound = "Sound(";
const string SoundAdd = "    #voice#";
const string TalkIDAdd = "#";

const string Str_MarkAuto = "    #Auto";

const string mark_talk_id = "#talk#";
const string mark_line_id = "#line#";

using Voices = unordered_map<int, string>;
auto GetVoiceDef(const string& path) {
	Voices voice;
	ifstream ifs(path);
	string name;
	int start, end;
	while (ifs >> name >> start >> end)
	{
		for (int i = start; i <= end; i++) {
			auto insrst = voice.insert({ i, name });
			if (!insrst.second) {
				cout << "Duplicate on " << i << ':' << name << endl;
			}
		}
	}
	ifs.close();
	return voice;
}
int main(int argc, char* argv[])
{
	if (argc <= 3) {
		cout << "Usage:\n"
			"\t" "ZA_Exporter_PY dir_out dir_py voicedef\n"
			<< endl;
		return 0;
	}

	string dir_py = argv[2];
	string dir_out = argv[1];
	string def = argv[3];

	const auto vdef = GetVoiceDef(def);

	Sora::MakeDirectory(dir_out);
	if (dir_out.length() > 0 && dir_out.back() != '\\') dir_out.push_back('\\');

	dir_py.push_back('\\');
	vector<string> fn_pys;
	Sora::SearchFiles(dir_py + "*" ATTR_PY, fn_pys);

	for (const auto &fn_py : fn_pys) {
		const string name = fn_py.substr(0, fn_py.rfind(ATTR_PY));
		cout << "处理" << fn_py << "..." << endl;

		ifstream ifs(dir_py + fn_py);
		ofstream ofs(dir_out + fn_py);
		int talk_id_cnt = 0;
		int line_id_cnt = 0;

		int line_no = 0;
		
		char buff[MAXCH_ONELINE + 1];
		string talk_type;
		int bra_cnt = 0;
		
		bool aut = false;
		string auto_mark;

		bool op2 = false;
		for(int line_no = 1;
			ifs.getline(buff, sizeof(buff));
			line_no++) 
		{
			string s = buff;
			if (!talk_type.empty()) {
				line_id_cnt++;

				if(s.find('"') == string::npos) {
					for (char c : s) {
						if (c == '(') ++bra_cnt;
						else if (c == ')') --bra_cnt;
					}
				}

				if (bra_cnt <= 0) {
					talk_type.clear();
					if (aut) auto_mark = Str_MarkAuto;
					aut = false;
					op2 = false;
				}

				size_t k = 0;
				while (!aut && k < s.length()) {
					while (k < s.length() && s[k] != '#') k++;
					if (s[k] == '#')  k++;
					while (s[k] >= '0' && s[k] <= '9') k++;
					if (s[k] == 'A') aut = true;
					k++;
				}

				auto idx = s.find(mark_line_id);
				if (idx != string::npos) {
					s = s.substr(0, idx);
					while (!s.empty() && s.back() == ' ') s.pop_back();
				}

				if (op2 && s.find('"') != string::npos) ofs << '\n';
				op2 = s.find(R"(\x02)") != string::npos;
			} //if (talk) 
			else {
				auto idx = s.find(Str_Sound);

				if (idx != string::npos) {
					int sound_id;
					if (sscanf(s.c_str() + idx + Str_Sound.length(), "%d", &sound_id)) {
						auto it = vdef.find(sound_id);
						if (it != vdef.end()) {
							s.append(SoundAdd).append(it->second);
						}
					}
				}
				else {
					for (const auto& search : TalkTypes) {
						auto idx = s.find(search);
						if (idx != string::npos) {
							bra_cnt = 1;
							line_id_cnt = 0;
							++talk_id_cnt;

							talk_type = search;

							ofs << s.substr(0, idx) << TalkIDAdd
								<< talk_type[0] << setfill('0') << setw(4) << setiosflags(ios::right) << talk_id_cnt << '\n';
						}

						idx = s.find(mark_talk_id);
						if (idx != string::npos) {
							s = s.substr(0, idx);
							while (!s.empty() && s.back() == ' ') s.pop_back();
						}
					}
				}
			}

			ofs << s << '\n';
			if (!auto_mark.empty()) {
				ofs << auto_mark << '\n';
				auto_mark.clear();
			}
		}

		ifs.close();
		ofs.close();
	}

	return 0;
}

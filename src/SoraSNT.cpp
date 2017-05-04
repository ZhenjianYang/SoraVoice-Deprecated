#include "SoraSNT.h"

#include <assert.h>

#define STR_4TBL "\t\t\t\t"
#define MAXCH_ONELINE 10000

using namespace std;

static const ExDataList emptyExDataList;

const string STR_SAY = "say";
const string STR_TEXT = "text";
const string STR_TALK = "talk";

const string Talks[] = { STR_TEXT, STR_SAY, STR_TALK };
const unsigned TextStart[] = { 1, 2, 3 };
const SNTItem::Type Types[] = { SNTItem::Type::Text, SNTItem::Type::Say, SNTItem::Type::Talk };
constexpr int TalksNum = std::extent<decltype(Talks)>::value;

void SNTItem::Output(std::ostream & ostr) const
{
	int i = 0;
	ostr << lines[0].content << '\n';

	for (int j = 0; j < TalksNum; j++) {
		if (type == Types[j]) {
			for (int i = 1; i < lines.size(); i++) {
				if (i == 1 && i < TextStart[j]) {
					ostr << lines[i].content << '\n';
				}
				else if (i == 2 && i < TextStart[j]) {
					ostr << STR_4TBL << lines[i].content << '\n';
				}
				else if (lines[i].content != "'" && lines[i].content != "\"") {
					ostr << STR_4TBL << lines[i].content << '\n';
				}
				else {
					ostr << lines[i].content << '\n';
				}
			}
			break;
		}
	}
}

SoraSNT::SoraSNT(std::istream& istr) : SoraSNT(istr, emptyExDataList) { }

SoraSNT::SoraSNT(std::istream & istr, const ExDataList & exDataList)
{
	char buff[MAXCH_ONELINE + 1];

	unsigned text_start = 0;
	for (int line_no = 1; istr.getline(buff, sizeof(buff)); line_no++) {
		auto it = exDataList.find(line_no);
		ExData* exData = it == exDataList.end() ? nullptr : it->second;

		string s = buff;
		size_t is = 0;
		if (text_start == 0 && (s[0] == ';' || s.find(".def") == 0)) {
			items.push_back(SNTItem(items.size()));
			items.back().lines.push_back({ s, exData });
		}
		else {
			while (is < s.length())
			{
				if (text_start == 0) {
					auto idx = string::npos;
					for (int i = 0; i < TalksNum; i++) {
						if ((idx = s.find(Talks[i], is)) != string::npos) {
							text_start = TextStart[i];

							if (idx > 0) {
								items.push_back(SNTItem(items.size()));
								items.back().lines.push_back({ s.substr(is, idx - is), exData });
							}

							items.push_back(SNTItem(items.size(), Types[i]));
							auto len = Talks[i].length();
							while (s[len] == ' ') len++;
							items.back().lines.push_back({ s.substr(idx, len), exData });

							is = idx + len;
							break;
						}
					}
				}

				if (text_start == 0) {
					items.push_back(SNTItem(items.size()));
					items.back().lines.push_back({ s.substr(is), exData });
					is += s.length();
				} // if (text_start == 0)
				else {
					if (items.back().lines.size() == 1 && items.back().lines.size() < text_start) {
						auto idx = s.find('\'', is);
						if (idx == string::npos) {
							items.back().lines.push_back({ s.substr(is), exData });
							is = s.length();
						}
						else {
							items.back().lines.push_back({ s.substr(is, idx - is), exData });
							is = idx;
						}
						while (items.back().lines.back().content.back() == '\t') items.back().lines.back().content.pop_back();
					}
					else if (items.back().lines.size() == 2 && items.back().lines.size() < text_start) {
						while (s[is] == '\t') is++;
						assert(s[is] == '\'');
						auto idx = s.find('"', is + 1);
						assert(idx != string::npos);
						items.back().lines.push_back({ s.substr(is, idx + 1 - is), exData });
						is = idx + 1;
					}
					else if (items.back().lines.size() == text_start) {
						while (s[is] == '\t') is++;
						assert(s[is] == '\'');
						items.back().lines.push_back({ s.substr(is, 1), exData });
						is = is + 1;
					}
					else if ((int)items.back().lines.size() > text_start) {
						auto idx = s.find('"', is);
						while (is < s.length() && is < idx) {
							if (s[is] == '\t') { is++; continue; };

							int cnt = 1;
							if (s[is] == '\\') {
								assert(s[is + 1] >= '0' && s[is + 1] <= '9');
								cnt = 2;
							}
							else if (s[is] < 0) {
								assert(s[is + 1]);
								cnt = 2;
							}

							const string& pre_line = items.back().lines.back().content;
							if (items.back().lines.size() == text_start + 1
								|| (s[is] != '\\' && pre_line.length() >= 2 && pre_line[pre_line.length() - 2] == '\\')) {
								items.back().lines.push_back({ "", exData });
							}

							for (int i = 0; i < cnt; i++, is++)
								items.back().lines.back().content.push_back(s[is]);
						}

						if (s[is] == '"') {
							items.back().lines.push_back({ s.substr(is, 1), exData });
							is = is + 1;
							text_start = 0;
						}
					}
				} //else (text_start == 0)
			} //while
		} //else
	}//for
}

void SoraSNT::Output(std::ostream & ostr) const
{
	for (const auto& item : items) {
		item.Output(ostr);
	}
}

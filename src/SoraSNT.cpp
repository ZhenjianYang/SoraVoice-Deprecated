#include "SoraSNT.h"

#include <assert.h>

#define STR_4TBL "\t\t\t\t"
#define MAXCH_ONELINE 10000

using namespace std;

static const ExDataList emptyExDataList;

const string STR_NORAML = "";
const string STR_TEXT = "text";
const string STR_SAY = "say";
const string STR_TALK = "talk";

constexpr int TS_NORMAL = 0;
constexpr int TS_TEXT = 1;
constexpr int TS_SAY = 2;
constexpr int TS_TALK = 3;

const char MARK_NORAML = '\0';
const char MARK_TEXT = 'X';
const char MARK_SAY = 'S';
const char MARK_TALK = 'T';

const ItemType ItemType::All::_normal(TS_NORMAL, STR_NORAML, MARK_NORAML);
const ItemType ItemType::All::_text(TS_TEXT, STR_TEXT, MARK_TEXT);
const ItemType ItemType::All::_say(TS_SAY, STR_SAY, MARK_SAY);
const ItemType ItemType::All::_talk(TS_TALK, STR_TALK, MARK_TALK);

constexpr PItemType SoraSNT::TalkTypes[];

void SNTItem::Output(std::ostream & ostr) const
{
	ostr << lines[0].content << '\n';

	for (size_t i = 1; i < lines.size(); i++) {
		if (i == 1 && i < type->TextStartLine) {
			ostr << lines[i].content << '\n';
		}
		else if (i == 2 && i < type->TextStartLine) {
			ostr << STR_4TBL << lines[i].content << '\n';
		}
		else if (lines[i].content != "'" && lines[i].content != "\"") {
			ostr << STR_4TBL << lines[i].content << '\n';
		}
		else {
			ostr << lines[i].content << '\n';
		}
	}
}

SoraSNT::SoraSNT(std::istream& istr) : SoraSNT(istr, emptyExDataList) { }

SoraSNT::SoraSNT(std::istream & istr, const ExDataList & exDataList)
{
	char buff[MAXCH_ONELINE + 1];

	PItemType type = AllItemTypes::Nomarl;
	for (int line_no = 1; istr.getline(buff, sizeof(buff)); line_no++) {
		auto it = exDataList.find(line_no);
		ExData* exData = it == exDataList.end() ? nullptr : it->second;

		string s = buff;
		size_t is = 0;
		if (type == AllItemTypes::Nomarl && (s[0] == ';' || s.find(".def") == 0)) {
			items.push_back(SNTItem(items.size()));
			items.back().lines.push_back({ s, exData });
		}
		else {
			while (is < s.length())
			{
				if (type == AllItemTypes::Nomarl) {
					auto idx = string::npos;
					for (auto it_type : TalkTypes) {
						if ((idx = s.find(it_type->Name, is)) != string::npos) {
							type = it_type;

							if (idx > 0) {
								items.push_back(SNTItem(items.size()));
								items.back().lines.push_back({ s.substr(is, idx - is), exData });
							}

							items.push_back(SNTItem(items.size(), type));
							auto len = type->Name.length();
							while (s[len] == ' ') len++;
							items.back().lines.push_back({ s.substr(idx, len), exData });

							is = idx + len;
							break;
						}
					}
				}

				if(is >= s.length()) continue;

				if (type == AllItemTypes::Nomarl) {
					items.push_back(SNTItem(items.size()));
					items.back().lines.push_back({ s.substr(is), exData });
					is += s.length();
				} // if (type == Types::Nomarl)
				else {
					if (items.back().lines.size() == 1 && items.back().lines.size() < type->TextStartLine) {
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
					else if (items.back().lines.size() == 2 && items.back().lines.size() < type->TextStartLine) {
						while (s[is] == '\t') is++;
						assert(s[is] == '\'');
						auto idx = s.find('"', is + 1);
						assert(idx != string::npos);
						items.back().lines.push_back({ s.substr(is, idx + 1 - is), exData });
						is = idx + 1;
					}
					else if (items.back().lines.size() == type->TextStartLine) {
						while (s[is] == '\t') is++;
						assert(s[is] == '\'');
						items.back().lines.push_back({ s.substr(is, 1), exData });
						is = is + 1;
					}
					else if (items.back().lines.size() > type->TextStartLine) {
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
							if (items.back().lines.size() == type->TextStartLine + 1
								|| (s[is] != '\\' && pre_line.length() >= 2 && pre_line[pre_line.length() - 2] == '\\')) {
								items.back().lines.push_back({ "", exData });
							}

							for (int i = 0; i < cnt; i++, is++)
								items.back().lines.back().content.push_back(s[is]);
						}

						if (s[is] == '"') {
							items.back().lines.push_back({ s.substr(is, 1), exData });
							is = is + 1;
							type = AllItemTypes::Nomarl;
						}
					}
				} //else (type == AllItemTypes::Nomarl)
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

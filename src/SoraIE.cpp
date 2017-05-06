#include "SoraIE.h"

#include <assert.h>
#include <memory>
#include <fstream>

#define STR_4TBL "\t\t\t\t"
#define MAXCH_ONELINE 10000

using namespace std;

static const ExDataList emptyExDataList;

const string STR_NORAML = "";
const string STR_SAY = "say";
const string STR_TALK = "talk";
const string STR_TEXT = "text";

constexpr int TS_NORMAL = 0;
constexpr int TS_SAY = 2;
constexpr int TS_TALK = 3;
constexpr int TS_TEXT = 1;

constexpr int MAX_TALKS_IN_SNT = 2000;

constexpr int MCODE_SAY = 1;
constexpr int MCODE_TALK = 2;
constexpr int MCODE_TEXT = 3;
constexpr int MAX_TALKS_IN_MBIN = 5000;

const char MARK_NORAML = '\0';
const char MARK_SAY = 'S';
const char MARK_TALK = 'T';
const char MARK_TEXT = 'X';

const ItemType ItemType::All::_normal(TS_NORMAL, STR_NORAML, MARK_NORAML);
const ItemType ItemType::All::_say(TS_SAY, STR_SAY, MARK_SAY);
const ItemType ItemType::All::_talk(TS_TALK, STR_TALK, MARK_TALK);
const ItemType ItemType::All::_text(TS_TEXT, STR_TEXT, MARK_TEXT);

constexpr PItemType SoraSNT::TalkTypes[];

using byte = unsigned char;

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

	items.reserve(MAX_TALKS_IN_SNT);

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

#define GET_INT(ptr) *(const int*)(ptr)
static const unordered_map<byte, int> scp_str_list = {
	//{ 0x01,0 },     //SCPSTR_CODE_LINE_FEED
	//{ 0x02,0 },     //SCPSTR_CODE_ENTER
	//{ 0x03,0 },     //SCPSTR_CODE_CLEAR
	//{ 0x04,0 },
	//{ 0x05,0 },
	//{ 0x06,0 },     //SCPSTR_CODE_05
	{ 0x07,1 },     //SCPSTR_CODE_COLOR
	//{ 0x09,0 },     //SCPSTR_CODE_09
	//{ 0x0A,0 },
	//{ 0x0D,0 },
	//{ 0x18,0 },
	{ 0x1F,2 },     //SCPSTR_CODE_ITEM
};

MbinTalk::MbinTalkList MbinTalk::GetMbinTalks(const std::string & mbin, bool utf8)
{
	ifstream ifs(mbin, ios::binary);
	ifs.seekg(0, ios::end);
	int len = (int)ifs.tellg();
	unique_ptr<byte[]> sbuff(new byte[len]);
	ifs.seekg(0, ios::beg);
	ifs.read((char*)sbuff.get(), len);
	ifs.close();
	const byte* const buff = sbuff.get();

	int ib = 0;
	int num = GET_INT(buff); ib += 4;
	assert(num < MAX_TALKS_IN_MBIN);

	vector<pair<int, int>> type_off_list;
	type_off_list.reserve(num);

	for (int i = 0; i < num; i++) {
		int type = GET_INT(buff + ib); ib += 4;
		int roff = GET_INT(buff + ib); ib += 4;

		if (type != 0) type_off_list.push_back({ type, roff + 4 + 8 * num });
	}

	MbinTalkList rst;
	rst.reserve(type_off_list.size());
	for (size_t i = 0; i < type_off_list.size(); i++) {
		assert(i == 0 || type_off_list[i].second >= rst.back().Offset + rst.back().Len);

		rst.push_back(MbinTalk());
		auto& talk = rst.back();
		talk.offset = type_off_list[i].second;
		auto& len = talk.len;

		talk.id = i + 1;
		len = 0;

		const int mbin_code = type_off_list[i].first;
		const byte* const ps = buff + talk.offset;

		len += 3;
		switch (mbin_code)
		{
		case MCODE_SAY:
			talk.type = AllItemTypes::Say;
			break;
		case MCODE_TALK:
			talk.type = AllItemTypes::Talk;
			talk.name = (char*)(ps + len);
			len += talk.name.length() + 1;
			break;
		case MCODE_TEXT:
			talk.type = AllItemTypes::Text;
		default:
			assert(false || "Unknow mbin code!");
			break;
		}

		talk.texts.push_back({ talk.offset + len, 0, "" });

		bool preSimbol = false;
		while (ps[len]) {
			if (ps[len] < 0x20) {
				if (ps[len] <= 3) {
					talk.texts.back().text.push_back('\\');
					talk.texts.back().text.push_back('0' + ps[len]);
					preSimbol = true;

					len++; talk.texts.back().len++;
				}
				else {
					if (preSimbol) talk.texts.push_back({ talk.offset + len, 0, "" });
					preSimbol = false;

					auto it = scp_str_list.find(ps[len]);
					int oplen = it == scp_str_list.end() ? 0 : it->second;

					talk.texts.back().text.append("[");
					char tb[4];
					for (int j = 0; j <= oplen; j++) {
						if (j != 0) talk.texts.back().text.push_back(' ');
						sprintf(tb, "%02X", ps[len]);
						talk.texts.back().text.append(tb);

						len++; talk.texts.back().len++;
					}
					talk.texts.back().text.append("]");
				}
			}
			else {
				if (preSimbol) talk.texts.push_back({ talk.offset + len, 0, "" });
				preSimbol = false;

				int cnt = 1;
				if (!utf8) {
					if (ps[len] >= 0x80) cnt = 2;
				}
				else {
					if (ps[len] >= 0xC0 && ps[len] < 0xE0) cnt = 2;
					else if(ps[len] >= 0xE0) cnt = 3;
				}

				for (int j = 0; j < cnt; j++) {
					talk.texts.back().text.push_back(ps[len]);
					len++; talk.texts.back().len++;
				}
			}
		}
		len++;
	}
	return rst;
}

#include "StringPatch.h"

#include <Utils/Log.h>
#include <Utils/Encoding.h>
#include <Patch/Pattern.h>

#include <string>
#include <fstream>
#include <unordered_map>

using namespace std;

using MapOffStrType = unordered_map<int, string>;
static MapOffStrType mOffStr;
static string cPattern;
static StringPatch::EditFun* pEditFun = nullptr;

inline static int getHex(const char* p) {
	if (p[0] != '\\' || p[1] != 'x') return -1;
	int hex = 0;
	p += 2;
	for (int i = 0; i < 2; i++) {
		if (*p >= '0' && *p <= '9') (hex *= 16) += *p - '0';
		else if (*p >= 'a' && *p <= 'f') (hex *= 16) += 10 + *p - 'a';
		else if (*p >= 'A' && *p <= 'F') (hex *= 16) += 10 + *p - 'f';
		else return -1;
	}
	return hex;
}

bool StringPatch::LoadStrings(const char * fileName)
{
	mOffStr.clear();
	ifstream ifs(fileName);
	if (!ifs) return false;

	string s;

	int offset = 0;
	string codename;
	string str;

	int cnt = 0;
	while (getline(ifs, s)) {
		if (s.empty() || s[0] == ';') {
			if (offset) {
				mOffStr[offset] = Encoding::Utf8ToStr(str, codename.c_str());
			}

			offset = 0;
			codename.clear();
			str.clear();
			cnt = 0;

			continue;
		}

		if (cnt == 0) {
			char * es;
			offset = strtol(s.c_str(), &es, 16);
		}
		else if (cnt == 1) {
			codename = s;
		}
		else {
			auto es = s.c_str();
			while (*es) {
				if (*es == '\\' && es[1] == 'n') {
					str.push_back('\n');
					es += 2;
				}
				else if (int ch = getHex(es); ch >= 0) {
					es += 4;
					str.push_back(char(ch));
				}
				else {
					str.push_back(*es);
					es += 1;
				}

			}
		}

		cnt++;
	}

	ifs.close();
	return true;
}

void StringPatch::SetPattern(const char * pattern)
{
	cPattern = pattern;
}

int StringPatch::Apply(void * start, int size, const char * pattern)
{
	if (mOffStr.empty()) return 0;
	if (!pattern && cPattern.empty() || !pEditFun) return 0;

	LOG("StringPatch::Apply, Start=0x%08X, Size=%d, Pattern=%s", (unsigned)start, size, pattern);

	const Pattern pt(pattern ? pattern : cPattern.c_str());
	
	int cnt = 0;
	for (unsigned char* p = (unsigned char*)start;
		p < (unsigned char*)start + size - pt.Legnth();
		/*empty*/) {
		if (unsigned(p) == 0x53850E) {
			p = p;
		}

		if (!pt.Check(p)) p++;
		else {
			p += pt.Legnth();
			int off = *(int*)p;
			auto it = mOffStr.find(off);
			if (it != mOffStr.end()) {
				pEditFun(p, (int)it->second.c_str());
				cnt++;
			}

			p += sizeof(int);
		}
	}

	LOG("StringPatch::Apply, total changed: %d", cnt);
	return cnt;
}

void StringPatch::SetEditFun(EditFun * editFun)
{
	pEditFun = editFun;
}

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
static const char* cPattern;

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
		if (s.empty() || s[0] == '#' || s[0] == ';') continue;

		if (cnt % 3 == 0) {
			char * es;
			offset = strtol(s.c_str(), &es, 16);
		}
		else if (cnt % 3 == 1) {
			codename = s;
		}
		else {
			auto es = s.c_str();
			while (*es) {
				if (*es == '\\') {
					if (es[1] == 'n') {
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
			mOffStr[offset] = Encoding::Utf8ToStr(str, codename.c_str());
		}
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
	if (!pattern && !cPattern) return 0;

	LOG("StringPatch::Apply, Start=0x%08X, Size=%d, Pattern=%s", (unsigned)start, size, pattern);

	const Pattern pt(pattern ? pattern : cPattern);
	
	int cnt = 0;
	for (unsigned char* p = (unsigned char*)start;
		p < (unsigned char*)start + size - pt.Legnth();
		/*empty*/) {
		if (!pt.Check(p)) p++;
		else {
			p += pt.Legnth();
			int off = *(int*)p;
			auto it = mOffStr.find(off);
			if (it != mOffStr.end()) {
				LOG("StringPatch::Apply 0x%08X, old=%s, new=%s", (unsigned)p,(char*)off ,it->second.c_str());
				*(int*)p = (int)it->second.c_str();
				cnt++;
			}

			p += sizeof(int);
		}
	}

	return cnt;
}

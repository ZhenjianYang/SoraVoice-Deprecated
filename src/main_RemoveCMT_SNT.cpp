#include <iostream>
#include <fstream>

#include "Common.h"

#define ATTR_SNT "._SN.txt"
#define MAXCH_ONELINE 10000

using namespace std;

int main(int argc, char* argv[])
{
	if (argc <= 1) {
		cout << "Usage:\n"
			"\t" "RemoveCMT_SNT dir_snt [dir_out]\n"
			"\n"
			"Default: dir_out = dir_snt.rc"
			<< endl;
		return 0;
	}

	string dir_snt = argv[1];  while (dir_snt.back() == '/' || dir_snt.back() == '\\') dir_snt.pop_back();
	string dir_out = argc > 2 ? argv[2] : dir_snt + ".rc";

	Sora::MakeDirectory(dir_out);
	if (dir_out.length() > 0 && dir_out.back() != '\\') dir_out.push_back('\\');

	dir_snt.push_back('\\');
	vector<string> fn_snts;
	Sora::SearchFiles(dir_snt + "*" ATTR_SNT, fn_snts);

	static char buff[MAXCH_ONELINE + 1];
	for (const auto &fn_snt : fn_snts) {
		const string name = fn_snt.substr(0, fn_snt.rfind(ATTR_SNT));
		cout << "处理" << fn_snt << "..." << endl;

		ifstream ifs(dir_snt + fn_snt);
		ofstream ofs(dir_out + fn_snt);

		while (ifs.getline(buff, sizeof(buff)))
		{
			for (int i = 0; buff[i]; i++) {
				if (buff[i] == '#' && buff[i + 1] == '#') {
					buff[i] = '\0';
					for (int j = i - 1; j >= 0 && buff[j] == '\t'; j--) {
						buff[j] = '\0';
					}
					break;
				}
			}
			ofs << buff << '\n';
		}

		ifs.close();
		ofs.close();

	}

	return 0;
}

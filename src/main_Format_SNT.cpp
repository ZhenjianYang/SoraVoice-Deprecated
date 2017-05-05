#include <iostream>
#include <fstream>
#include <iomanip>
#include <assert.h>

#include "Common.h"
#include "SoraSNT.h"

#define ATTR_SNT "._SN.txt"

using namespace std;

int main(int argc, char* argv[])
{
	if (argc <= 1) {
		cout << "Usage:\n"
			"\t" "Foramt_SNT dir_snt [dir_out]\n"
			"\n"
			"Default: dir_out = dir_snt.fmt"
			<< endl;
		return 0;
	}

	string dir_snt = argv[1];  while (dir_snt.back() == '/' || dir_snt.back() == '\\') dir_snt.pop_back();
	string dir_out = argc > 2 ? argv[2] : dir_snt + ".fmt";

	Sora::MakeDirectory(dir_out);
	if (dir_out.length() > 0 && dir_out.back() != '\\') dir_out.push_back('\\');

	dir_snt.push_back('\\');
	vector<string> fn_snts;
	Sora::SearchFiles(dir_snt + "*" ATTR_SNT, fn_snts);
	for (const auto &fn_snt : fn_snts) {
		const string name = fn_snt.substr(0, fn_snt.rfind(ATTR_SNT));
		cout << "处理" << fn_snt << "..." << endl;

		ifstream ifs(dir_snt + fn_snt);
		ofstream ofs(dir_out + fn_snt);

		SoraSNT snt(ifs);
		snt.Output(ofs);

		ifs.close();
		ofs.close();

	}

	return 0;
}

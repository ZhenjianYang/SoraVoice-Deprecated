#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <unordered_map>
#include <algorithm>

#include "common.h"

#define ATTR_PY ".py"
#define ATTR_IPT ".txt"
#define REP_NAME "import_py_report.txt"

#define MAXCH_ONELINE 2048
#define MAX_LINENO_LEN 9

#define MAX_LINENO 0x7FFFFFFF

using namespace std;

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
			"\t" "ZA_Importer_PY  dir_py_new dir_py_old dir_py_out dir_bin_out\n"
			<< endl;
		return 0;
	}

	string dir_out = argv[1];
	string dir_py = argv[2];
	string dir_py_out = argv[3];
	string dir_bin_out = argv[4];

	Sora::MakeDirectory(dir_out);
	if (dir_out.length() > 0 && dir_out.back() != '\\') dir_out.push_back('\\');
	if (dir_py.length() > 0 && dir_py.back() != '\\') dir_py.push_back('\\');
	if (dir_py_out.length() > 0 && dir_py_out.back() != '\\') dir_py_out.push_back('\\');
	if (dir_bin_out.length() > 0 && dir_bin_out.back() != '\\') dir_bin_out.push_back('\\');

	vector<string> fn_pys;
	Sora::SearchFiles(dir_py + "*" ATTR_PY, fn_pys);

	ofstream ofs_rp(REP_NAME);
	for (const auto &fn_py : fn_pys) {
		string name = fn_py.substr(0, fn_py.rfind(ATTR_PY));
		cout << "处理" << fn_py << "..." << endl;

		ifstream ifs(dir_py + fn_py);
		ofstream ofs(dir_out + fn_py);

		char buff[MAXCH_ONELINE + 1];

		for (int line_no = 1;
			ifs.getline(buff, sizeof(buff));
			line_no++)
		{

		}

		ifs.close();
		ofs.close();
	}

	return 0;
}

#include "common.h"

#include <Windows.h>
#include <direct.h> 

using namespace std;

void Sora::SearchFiles(const std::string & seachFileName, std::vector<std::string>& out_filesFound, bool nameOnly /* = true */)
{
	WIN32_FIND_DATA wfdp;
	HANDLE hFindp = FindFirstFile(seachFileName.c_str(), &wfdp);
	string father;
	auto idx = seachFileName.find_last_of("\\/");
	if (idx != string::npos) {
		father = seachFileName.substr(0, idx + 1);
	}
	if (hFindp != INVALID_HANDLE_VALUE) {
		do
		{
			if (!(wfdp.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				if(nameOnly)
					out_filesFound.push_back(wfdp.cFileName);
				else
					out_filesFound.push_back(father + wfdp.cFileName);
			}
		} while (FindNextFile(hFindp, &wfdp));
		FindClose(hFindp);
	}
}

bool Sora::MakeDirectory(const std::string & dir)
{
	string tmp = dir;
	size_t idx = 0;

	while (true) {
		idx = tmp.find_first_of("\\/", idx);
		if (idx != string::npos) {
			tmp[idx] = 0;
			_mkdir(tmp.c_str());
			tmp[idx] = '/';
			idx = tmp.find_first_not_of("\\/", idx);
		}
		else {
			return !_mkdir(tmp.c_str());
		}
	}
}

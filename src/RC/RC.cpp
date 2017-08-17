#include "Rc.h"

#define NOMINMAX
#include <Windows.h>

#include <memory>
#include <fstream>
#include <unordered_map>
#include <string>
#include <algorithm>

using namespace std;

class RCImpl : public RC
{
public:
	virtual const char* First() const { return data.get(); };
	virtual unsigned Size() const { return size; };

	virtual ~RCImpl() { }
	RCImpl(const char* name);

private:
	unique_ptr<char[]> data;
	unsigned size = 0;
};

using RcTableType = unordered_map<string, pair<int, string>>;
static RcTableType _rcTable;
static HINSTANCE _mh;

RCImpl::RCImpl(const char * name) {
	ifstream ifs(name, ios::binary);
	if (ifs) {
		ifs.seekg(0, ios::end);
		size = std::min(MaxRCSize, (unsigned)ifs.tellg());
		ifs.seekg(0, ios::beg);

		data = make_unique<char[]>(size);
		ifs.read(data.get(), size);
		ifs.close();
	}

	if (!data) {
		auto it = _rcTable.find(name);
		if (it != _rcTable.end()) {
			const auto& idtype = it->second;
			HRSRC hrsrc = FindResourceA(_mh, MAKEINTRESOURCE(idtype.first), idtype.second.c_str());
			if (hrsrc) {
				HGLOBAL h = LoadResource(_mh, hrsrc);
				if (h) {
					char* first = (char*)LockResource(h);
					if (first) {
						size = std::min(MaxRCSize, (unsigned)SizeofResource(_mh, hrsrc));

						data = make_unique<char[]>(size);
						memcpy(data.get(), first, size);
					}
				}
			}
		}
	}
}


RC * RC::Get(const char * name) {
	RCImpl* rc = new RCImpl(name);
	if (!rc->First()) {
		delete rc;
		rc = nullptr;
	}
	return rc;
}

void RC::SetRcTable(RcItem * rcItemList) {
	_rcTable.clear();

	if (rcItemList) {
		while (rcItemList->name && rcItemList->type) {
			_rcTable[rcItemList->name] = {
				rcItemList->id,
				rcItemList->type
			};
			rcItemList++;
		}
	}
}

void RC::SetModuleHandle(void * mh) {
	_mh = (decltype(_mh))mh;
}


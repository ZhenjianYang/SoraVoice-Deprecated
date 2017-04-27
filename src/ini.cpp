#include "ini.h"
#include <fstream>
#include <unordered_map>
#include <memory>
#include <vector>
#include <string>

using namespace std;

#define MAXCH_ONELINE 2048

using VectorKeys = vector<string>;
using VectorValues = vector<string>;
using MapKeyIndex = unordered_map<string, int>;

class GroupImpl : private INI::Group
{
	friend class INI;
	friend struct INIData;
private:
	static const GroupImpl InValidGroup;

	bool valid;
	string name;
	VectorKeys values;
	VectorValues keys;
	MapKeyIndex map_name_idx;

	bool Valid() const override { return valid; }
	int Num() const override { return values.size(); }
	const char * Name() const override { return Valid() ? name.c_str() : nullptr; }
	const char * GetKey(int index) const override
	{
		if (!Valid() || index < 0 || index >(int)keys.size()) return nullptr;
		return keys[index].c_str();
	}
	const char * GetValue(int index) const override
	{
		if (!Valid() || index < 0 || index >(int)values.size()) return nullptr;
		return values[index].c_str();
	}
	const char * GetValue(const char * key) const override
	{
		if (!Valid() || !key) return nullptr;

		auto it = map_name_idx.find(key);
		if (it == map_name_idx.end()) return nullptr;

		return values[it->second].c_str();
	}

	GroupImpl(const string& name = "", bool valid = true) : valid(valid), name(name) { }
};

const GroupImpl GroupImpl::InValidGroup("", false);
using PtrGroup = unique_ptr<GroupImpl>;
using VectorGroup = vector<PtrGroup>;

struct INIData
{
	VectorGroup groups;
	MapKeyIndex map_name_idx;
};
using PtrINIData = unique_ptr<INIData>;

int INI::Num() const
{
	INIData* iniData = (INIData*)data;
	if (!Valid()) return 0;

	return iniData->groups.size();
}

const INI::Group & INI::GetGroup(int index) const
{
	INIData* iniData = (INIData*)data;
	if (!Valid() || index < 0 || index > (int)iniData->groups.size()) return GroupImpl::InValidGroup;

	return *iniData->groups[index];
}

const INI::Group & INI::GetGroup(const char * name) const
{
	INIData* iniData = (INIData*)data;
	if (!Valid()) return GroupImpl::InValidGroup;

	if (name == nullptr || name[0] == '\0') return *iniData->groups[0];

	auto it = iniData->map_name_idx.find(name);
	if (it == iniData->map_name_idx.end()) return GroupImpl::InValidGroup;

	return *iniData->groups[it->second];
}

bool INI::Open(const char * fileName)
{
	ifstream ifs(fileName);
	Open(ifs);
	ifs.close();

	return Valid();
}

bool INI::Open(std::istream & is)
{
	delete (INIData*)data;
	data = nullptr;

	if (!is) return false;

	char buff[MAXCH_ONELINE];
	PtrINIData iniData(new INIData);

	iniData->groups.push_back(PtrGroup(new GroupImpl));
	iniData->map_name_idx[""] = 0;
	GroupImpl* group = iniData->groups.back().get();

	while (is.getline(buff, sizeof(buff)))
	{
		if (buff[0] == 0 || buff[0] == '#' || buff[0] == ';') continue;
		buff[sizeof(buff) - 1] = 0;

		if (buff[0] == '[') {
			int idx = 1;
			while (buff[idx] == ' ' || buff[idx] == '\t') idx++;
			string name = buff + idx;
			while (!name.empty() && (name.back() == ' ' || name.back() == '\t' || name.back() == '\r' || name.back() == '\n')) name.pop_back();
			if (name.back() != ']') continue;
			name.pop_back();

			while (!name.empty() && (name.back() == ' ' || name.back() == '\t' || name.back() == '\r' || name.back() == '\n')) name.pop_back();
			auto it = iniData->map_name_idx.find(name);
			if (it == iniData->map_name_idx.end()) {
				iniData->map_name_idx[name] = iniData->groups.size();
				iniData->groups.push_back(PtrGroup(new GroupImpl(name)));
				group = iniData->groups.back().get();
			}
			else {
				group = iniData->groups[it->second].get();
			}
		}
		else {
			string key, value;
			char* p = buff;
			while (*p && (*p == ' ' || *p == '\t')) p++;

			while (*p && *p != '=') key.push_back(*p++);
			if (*p++ != '=') continue;

			while (!key.empty() && (key.back() == ' ' || key.back() == '\t' || key.back() == '\r' || key.back() == '\n')) key.pop_back();
			if (key.empty()) continue;

			while (*p && (*p == ' ' || *p == '\t')) p++;
			while (*p) value.push_back(*p++);
			while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r' || value.back() == '\n')) value.pop_back();

			auto it = group->map_name_idx.find(key);
			if (it == group->map_name_idx.end()) {
				group->map_name_idx[key] = group->values.size();
				group->values.push_back(value);
				group->keys.push_back(key);
			}
			else {
				group->values[it->second] = value;
			}
		}
	} //while()

	this->data = iniData.release();
	return true;
}

INI::~INI()
{
	delete (INIData*)data;
}

INI::INI(INI && _Right)
{
	data = _Right.data;
	_Right.data = nullptr;
}

INI& INI::operator=(INI && _Right)
{
	delete (INIData*)data;
	data = _Right.data;
	_Right.data = nullptr;

	return *this;
}

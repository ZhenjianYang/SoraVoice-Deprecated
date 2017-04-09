#include "config.h"

#include <fstream>
#include <map>
#include <iomanip>

using namespace std;

#define MAXCH_ONELINE 1024

using KeyValue = map<string, string>;

#define GET_VALUE(name, kv) _getValue(name, kv, STR_##name)
#define GET_VALUE_MAXFIX(name, kv) {_getValue(name, kv, STR_##name);\
				if(name < 0) name = 0; \
				if(name > MAX_##name) name = MAX_##name;}
#define GET_VALUE_BOOLFIX(name, kv) {_getValue(name, kv, STR_##name); if(name) name = 1;}

#define OUTPUT_VALUE(name, ofs) ofs << CMT_##name << '\n' << STR_##name << " = " << name << '\n' << '\n'
#define OUTPUT_VALUE_WFMT(name, ofs, format) ofs << CMT_##name << '\n' << STR_##name << " = " << format << name << '\n' << '\n'

#define SET_DEFAULT(name) name = DFT_##name
#define SET_DEFAULT_STR(name) strcpy(name, DFT_##name)

static bool _getValue(int& var, const KeyValue& kv, const char* name) {
	auto it = kv.find(name);
	if (it == kv.end()) return false;

	int rad = 10;
	if (it->second[0] == '0' && (it->second[1] == 'x' || it->second[1] == 'X')) {
		rad = 16;
	}
	char *p;
	var = (int)std::strtoul(it->second.c_str(), &p, rad);
	return true;
}

template<typename ArrayType, typename = std::enable_if<std::is_array<ArrayType>::value>::type>
static bool _getValue(ArrayType &var, const KeyValue& kv, const char* name) {

	auto it = kv.find(name);
	if (it == kv.end()) return false;

	for (int i = 0; i < (int)it->second.length() && i < sizeof(var) / sizeof(&var); i++) {
		var[i] = it->second[i];
	}
	var[sizeof(var) / sizeof(&var) - 1] = 0;

	return true;
}

bool Config::LoadConfig(const char * configFn)
{
	load_default();

	ifstream ifs(configFn);

	if (!ifs) return false;
	KeyValue kv;

	char buff[MAXCH_ONELINE];
	while (ifs.getline(buff, sizeof(buff)))
	{
		if (buff[0] == 0 || buff[0] == '#' || buff[0] == ';') continue;
		buff[sizeof(buff) - 1] = 0;

		string key, value;
		
		char* p = buff;
		while (*p && (*p == ' ' || *p == '\t')) p++;

		while (*p && *p != '=') key.push_back(*p++);
		if (*p++ != '=') continue;

		while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) key.pop_back();
		if (key.empty()) continue;

		while (*p && (*p == ' ' || *p == '\t')) p++;
		while (*p) value.push_back(*p++);
		while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) value.pop_back();

		kv.insert({ key, value });
	}

	GET_VALUE_MAXFIX(Volume, kv);

	GET_VALUE_MAXFIX(AutoPlay, kv);
	GET_VALUE(WaitTimePerChar, kv);
	GET_VALUE(WaitTimeDialog, kv);
	GET_VALUE(WaitTimeDialogVoice, kv);

	GET_VALUE_BOOLFIX(SkipVoice, kv);
	GET_VALUE_BOOLFIX(DisableDialogSE, kv);
	GET_VALUE_BOOLFIX(DisableDududu, kv);

	GET_VALUE_MAXFIX(ShowInfo, kv);
	GET_VALUE(FontName, kv);
	GET_VALUE(FontColor, kv);

	GET_VALUE_BOOLFIX(EnableKeys, kv);
	GET_VALUE_BOOLFIX(SaveChange, kv);

	if (AutoPlay) SkipVoice = 1;

	return true;
}

bool Config::SaveConfig(const char * configFn) const
{
	ofstream ofs(configFn);
	if (!ofs) return false;
	
	OUTPUT_VALUE(Volume, ofs);
	ofs << '\n';

	OUTPUT_VALUE(AutoPlay, ofs);
	OUTPUT_VALUE(WaitTimePerChar, ofs);
	OUTPUT_VALUE(WaitTimeDialog, ofs);
	OUTPUT_VALUE(WaitTimeDialogVoice, ofs);
	ofs << '\n';

	OUTPUT_VALUE(SkipVoice, ofs);
	OUTPUT_VALUE(DisableDialogSE, ofs);
	OUTPUT_VALUE(DisableDududu, ofs);
	OUTPUT_VALUE(ShowInfo, ofs);
	ofs << '\n';

	OUTPUT_VALUE(FontName, ofs);
	OUTPUT_VALUE_WFMT(FontColor, ofs, "0x" << setfill('0') << setw(8) << setiosflags(ios::right | ios::uppercase) << hex);
	ofs << '\n';

	OUTPUT_VALUE(EnableKeys, ofs);
	OUTPUT_VALUE(SaveChange, ofs);

	ofs.close();
	return true;
}

void Config::load_default(bool all)
{
	SET_DEFAULT(Volume);

	SET_DEFAULT(AutoPlay);
	SET_DEFAULT(WaitTimePerChar);
	SET_DEFAULT(WaitTimeDialog);
	SET_DEFAULT(WaitTimeDialogVoice);

	SET_DEFAULT(SkipVoice);
	SET_DEFAULT(DisableDialogSE);
	SET_DEFAULT(DisableDududu);

	SET_DEFAULT(ShowInfo);
	SET_DEFAULT_STR(FontName);
	SET_DEFAULT(FontColor);

	if (all) {
		SET_DEFAULT(EnableKeys);
		SET_DEFAULT(SaveChange);
	}
}

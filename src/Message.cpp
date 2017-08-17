#include "Message.h"

#include <RC/RC.h>
#include <Utils/INI.h>

#include <Windows.h>

#include <memory>
#include <sstream>
#include <string>

#define RCNAME_EN "voice/Message_en.ini"
#define RCNAME_CN "voice/Message_zh.ini"
#define RCNAME_DFTLAN RCNAME_EN

#define RCNAME_DFT "voice/Message.ini"

#define SET_MESSAGE(top, name) { \
	static std::string val_##name;\
	auto value = top.GetValue(#name);\
	if (value) {\
		val_##name.clear();\
		while (*value) {\
			if (*value == '\\' && *(value + 1) == 'n') { val_##name.push_back('\n'); value += 2;}\
			else { val_##name.push_back(*value);value++;} \
		}\
		CMessage::_Message.name = val_##name.c_str();\
	}\
}

#define SET_MESSAGE_LIST(top, name, ...) {\
	const char* const _list[] = { __VA_ARGS__ };\
	static_assert(std::extent_v<decltype(_list)> == std::extent_v<decltype(CMessage::_Message.name)>,\
		"Length of " #name " Not Match!");\
	for (unsigned _i = 0; _i < std::extent_v<decltype(_list)>; _i++) {\
		CMessage::_Message.name[_i] = _list[_i];\
	}\
}

#define SET_MESSAGE_CMT(top, name) { \
	static std::string val_##name;\
	auto value = top.GetValue("CMT_" #name);\
	if (value) {\
		val_##name.clear();\
		while (*value) {\
			if (*value == '\\' && *(value + 1) == 'n') { val_##name.push_back('\n'); value += 2;}\
			else { val_##name.push_back(*value);value++;} \
		}\
		CMessage::_Message.CMT.name = val_##name.c_str();\
	}\
}

void CMessage::LoadMessage()
{
	LoadMessage(RCNAME_DFTLAN);

	if (LoadMessage(RCNAME_DFT)) return;

	auto lcid = GetSystemDefaultLCID();
	if (lcid == 0x0804) {
		if (LoadMessage(RCNAME_CN)) return;
	}
}

bool CMessage::LoadMessage(const char * resName)
{
	INI ini; {
		std::unique_ptr<RC> rc(RC::Get(resName));
		if (!rc || !rc->First()) {
			return false;
		}
		std::stringstream ss(std::string(rc->First(), rc->Size()));
		ini.Open(ss);
	}

	auto &top = ini.GetTopGroup();
	if (top.Num() == 0) return false;

	SET_MESSAGE(top, Version);
	SET_MESSAGE(top, CurrentTitle);
	SET_MESSAGE(top, Volume);
	SET_MESSAGE(top, Mute);
	SET_MESSAGE(top, OriginalVoice);
	SET_MESSAGE(top, OriVolumePercent);
	SET_MESSAGE(top, Reset);
	SET_MESSAGE(top, AutoPlay);
	SET_MESSAGE(top, SkipVoice);
	SET_MESSAGE(top, DisableDialogSE);
	SET_MESSAGE(top, DisableDududu);
	SET_MESSAGE(top, AutoPlayMark);
	SET_MESSAGE(top, Title_Sora);
	SET_MESSAGE(top, Title_ZA);
	SET_MESSAGE(top, On);
	SET_MESSAGE(top, Off);
	SET_MESSAGE(top, OriEvoVoiceBoth);
	SET_MESSAGE(top, EvoVoicOnly);
	SET_MESSAGE(top, OriVoiceOnly);
	SET_MESSAGE(top, AutoPlayVoice);
	SET_MESSAGE(top, AutoPlayAll);
	SET_MESSAGE(top, ShowInfo);
	SET_MESSAGE(top, ShowInfoAuto);

	SET_MESSAGE_LIST(top, Switch, CMessage::_Message.Off, CMessage::_Message.On);
	SET_MESSAGE_LIST(top, OriginalVoiceSwitch, CMessage::_Message.OriEvoVoiceBoth, CMessage::_Message.EvoVoicOnly, CMessage::_Message.OriVoiceOnly);
	SET_MESSAGE_LIST(top, AutoPlaySwitch, CMessage::_Message.Off, CMessage::_Message.AutoPlayVoice, CMessage::_Message.AutoPlayAll);
	SET_MESSAGE_LIST(top, ShowInfoSwitch, CMessage::_Message.Off, CMessage::_Message.On, CMessage::_Message.ShowInfoAuto);

	SET_MESSAGE_CMT(top, Volume);
	SET_MESSAGE_CMT(top, OriginalVoice);
	SET_MESSAGE_CMT(top, OriVolumePercent);
	SET_MESSAGE_CMT(top, AutoPlay);
	SET_MESSAGE_CMT(top, WaitTimePerChar);
	SET_MESSAGE_CMT(top, WaitTimeDialog);
	SET_MESSAGE_CMT(top, WaitTimeDialogVoice);
	SET_MESSAGE_CMT(top, SkipVoice);
	SET_MESSAGE_CMT(top, DisableDialogSE);
	SET_MESSAGE_CMT(top, DisableDududu);
	SET_MESSAGE_CMT(top, ShowInfo);
	SET_MESSAGE_CMT(top, FontName);
	SET_MESSAGE_CMT(top, FontColor);
	SET_MESSAGE_CMT(top, SaveChange);
	SET_MESSAGE_CMT(top, EnableKeys_ZA);
	SET_MESSAGE_CMT(top, EnableKeys_Sora);

#ifdef ZA
	CMessage::_Message.Title = CMessage::_Message.Title_ZA;
	CMessage::_Message.CMT.EnableKeys = CMessage::_Message.CMT.EnableKeys_ZA;
#else
	CMessage::_Message.Title = CMessage::_Message.Title_Sora;
	CMessage::_Message.CMT.EnableKeys = CMessage::_Message.CMT.EnableKeys_Sora;
#endif // ZA

	return true;
}

CMessage CMessage::_Message;
const CMessage& Message = CMessage::_Message;

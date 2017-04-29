#pragma once

namespace Message {
	constexpr const char Title_Sora[] =  u8"《空之轨迹》语音补丁";
	constexpr const char Title_ZA[] =  u8"《零·碧之轨迹》语音补丁";
#ifdef ZA
	constexpr const char *Title = Title_ZA;
#else
	constexpr const char *Title = Title_Sora;
#endif // ZA

	constexpr const char Version[] =  u8"版本 %s";
	constexpr const char VersionNum[] =  u8"0.5.0";

	constexpr const char GameTitle[] =  u8"当前游戏为：%s";

	constexpr const char On[] =  u8"开";
	constexpr const char Off[] =  u8"关";
	constexpr const char* Switch[] = { Off, On };
	
	constexpr const char Volume[] =  u8"音量：%d";
	constexpr const char Mute[] =  u8"静音";

	constexpr const char* OriginalVoiceSwitch[] = { Off, On };
	constexpr const char OriginalVoice[] = u8"禁用原有剧情语音：%s";

	constexpr const char Reset[] =  u8"重置设置";
	
	constexpr const char AutoPlay[] =  u8"自动播放：%s";
	constexpr const char AutoPlayVoice[] =  u8"有语音时开启";
	constexpr const char AutoPlayAll[] =  u8"一律开启";
	constexpr const char* AutoPlaySwitch[] = { Off, AutoPlayVoice, AutoPlayAll };

	constexpr const char SkipVoice[] =  u8"对话框关闭时终止语音：%s";
	constexpr const char DisableDialogSE[] =  u8"禁用对话框关闭音效：%s";
	constexpr const char DisableDududu[] =  u8"禁用文字显示音效：%s";
	
	constexpr const char ShowInfo[] =  u8"信息显示：%s";
	constexpr const char ShowInfoAuto[] =  u8"开(显示自动播放符号)";
	constexpr const char* ShowInfoSwitch[] = {Off, On, ShowInfoAuto};
	
	constexpr const char AutoPlayMark[] =  u8"AUTO";
};



#pragma once

namespace Message {
	constexpr const char Title_Sora[] = "《空之轨迹》语音补丁";
	constexpr const char Title_ZA[] = "《零·碧之轨迹》语音补丁";
#ifdef ZA
	constexpr const char *Title = Title_ZA;
#else
	constexpr const char *Title = Title_Sora;
#endif // ZA

	constexpr const char Version[] = "版本 %s";
	constexpr const char VersionNum[] = "0.4.1";

	constexpr const char GameTitle[] = "本游戏为：%s";

	constexpr const char On[] = "开";
	constexpr const char Off[] = "关";
	constexpr const char* Switch[] = { Off, On };
	
	constexpr const char Volume[] = "音量：%d";
	constexpr const char Mute[] = "静音";

	constexpr const char Reset[] = "重置设置";
	
	constexpr const char AutoPlay[] = "自动播放：%s";
	constexpr const char AutoPlayVoice[] = "有语音时开启";
	constexpr const char AutoPlayAll[] = "一律开启";
	constexpr const char* AutoPlaySwitch[] = { Off, AutoPlayVoice, AutoPlayAll };

	constexpr const char SkipVoice[] = "对话框关闭时终止语音：%s";
	constexpr const char DisableDialogSE[] = "禁用对话框关闭音效：%s";
	constexpr const char DisableDududu[] = "禁用文字显示音效：%s";
	
	constexpr const char ShowInfo[] = "信息显示：%s";
	constexpr const char ShowInfoAuto[] = "开(显示自动播放符号)";
	constexpr const char* ShowInfoSwitch[] = {Off, On, ShowInfoAuto};
	
	constexpr const char AutoPlayMark[] = "AUTO";
};



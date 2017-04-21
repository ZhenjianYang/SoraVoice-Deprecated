#pragma once

class Message {
public:
	static constexpr char Title[] = "《空之轨迹》语音补丁";
	static constexpr char Version[] = "版本 %s";
	static constexpr char VersionNum[] = "0.4.1";

	static constexpr char On[] = "开";
	static constexpr char Off[] = "关";
	static constexpr const char* Switch[] = { Off, On };
	
	static constexpr char Volume[] = "音量：%d";
	static constexpr char Mute[] = "静音";

	static constexpr char Reset[] = "重置设置";
	
	static constexpr char AutoPlay[] = "自动播放：%s";
	static constexpr char AutoPlayVoice[] = "有语音时开启";
	static constexpr char AutoPlayAll[] = "一律开启";
	static constexpr const char* AutoPlaySwitch[] = { Off, AutoPlayVoice, AutoPlayAll };

	static constexpr char SkipVoice[] = "对话框关闭时终止语音：%s";
	static constexpr char DisableDialogSE[] = "禁用对话框关闭音效：%s";
	static constexpr char DisableDududu[] = "禁用文字显示音效：%s";
	
	static constexpr char ShowInfo[] = "信息显示：%s";
	static constexpr char ShowInfoAuto[] = "开(显示自动播放符号)";
	static constexpr const char* ShowInfoSwitch[] = {Off, On, ShowInfoAuto};
	
	static constexpr char AutoPlayMark[] = "AUTO";
};



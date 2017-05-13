#pragma once

namespace Message {
	constexpr const char Title_Sora[]				= u8"《空之轨迹》语音补丁";
	constexpr const char Title_ZA[]					= u8"《零·碧之轨迹》语音补丁";
#ifdef ZA
	constexpr const char *Title						=	Title_ZA;
#else
	constexpr const char *Title						=	Title_Sora;
#endif // ZA

	constexpr const char Version[]					= u8"版本 %s";
	constexpr const char VersionNum[]				= u8"0.5.0";

	constexpr const char GameTitle[]				= u8"当前游戏为：%s";

	constexpr const char On[]						= u8"开";
	constexpr const char Off[]						= u8"关";
	constexpr const char* Switch[]					=	{ Off, On };

	constexpr const char Volume[]					= u8"音量：%d";
	constexpr const char Mute[]						= u8"静音";

	constexpr const char OriEvoVoiceBoth[]			= u8"与进化版语音同时播放";
	constexpr const char EvoVoicOnly[]				= u8"仅播放进化版语音(禁用原有语音)";
	constexpr const char OriVoiceOnly[]				= u8"仅播放原有语音(不播放进化版语音)";
	constexpr const char* OriginalVoiceSwitch[]		=	{ OriEvoVoiceBoth, EvoVoicOnly, OriVoiceOnly };
	constexpr const char OriginalVoice[]			= u8"存在原有剧情语音时：%s";

	constexpr const char Reset[]					= u8"重置设置";

	constexpr const char AutoPlay[]					= u8"自动播放：%s";
	constexpr const char AutoPlayVoice[]			= u8"有语音时开启";
	constexpr const char AutoPlayAll[]				= u8"一律开启";
	constexpr const char* AutoPlaySwitch[]			=	{ Off, AutoPlayVoice, AutoPlayAll };

	constexpr const char SkipVoice[]				= u8"对话框关闭时终止语音：%s";
	constexpr const char DisableDialogSE[]			= u8"禁用对话框关闭音效：%s";
	constexpr const char DisableDududu[]			= u8"禁用文字显示音效：%s";

	constexpr const char ShowInfo[]					= u8"信息显示：%s";
	constexpr const char ShowInfoAuto[]				= u8"开(显示自动播放符号)";
	constexpr const char* ShowInfoSwitch[]			=	{Off, On, ShowInfoAuto};

	constexpr const char AutoPlayMark[]				= u8"AUTO";

	namespace CMT {
		constexpr const char Volume[]				= u8"#音量，范围为0~100，默认为100(最大值)"
													;

		constexpr const char OriginalVoice[]		= u8"#当游戏中的原有剧情语音与进化版语音同时存在时：\n"
														";    0    同时播放(即不作任何特殊处理)\n"
														";    1    仅播放进化版语音(禁用原有语音)\n"
														";    2    仅播放原版语音(不播放进化版语音)\n"
														"#默认为1\n"
														"###注意：此项配置为2时，原版语音的播放是由本补丁进行的;        ###\n"
														"###      这意味着对于该条语音，游戏原本的音量配置将不会起作用。###"
													;

		constexpr const char AutoPlay[]				= u8"#自动播放。模式罗列如下：\n"
														";    0    关闭\n"
														";    1    仅在有语音时启用自动播放\n"
														";    2    对所有字幕启用自动播放\n"
														";接下来的3个参数为设置自动播放时间的选项。\n"
														"#默认为2\n"
														"###注意：当启用此选项时，会强制启用选项SkipVoice。###"
													;

		constexpr const char WaitTimePerChar[]		= u8"#无语音时，为每个字分配的等待时间。默认为60(单位毫秒，下同)"
													;

		constexpr const char WaitTimeDialog[]		= u8"#无语音时，为对话框分配的额外等待时间。默认为800。\n"
														"#则在默认配置下，一个20字的对话框的等待时间为800+20x60=2000毫秒"
													;

		constexpr const char WaitTimeDialogVoice[]	= u8"#有语音时，语音播放结束后额外的等待时间。默认为500"
													;

		constexpr const char SkipVoice[]			= u8"#对话框关闭时，终止语音。默认为1(启用此项功能)\n"
														"###注意：当关闭此选项时，会强制关闭选项AutoPlay。###"
													;

		constexpr const char DisableDialogSE[]		= u8"#在语音播放时，禁用对话框关闭(切换)时的音效。默认为1(启用此项功能)"
													;

		constexpr const char DisableDududu[]		= u8"#在语音播放时，禁用文字显示的嘟嘟声。默认为1(启用此项功能)"
													;

		constexpr const char ShowInfo[]				= u8"#显示信息。模式罗列如下：\n"
														";    0    关闭，配置变化时，不显示信息\n"
														";    1    配置变化时，显示信息\n"
														";    2    配置变化时，显示信息；同时在自动播放的情况下显示自动播放的符号\n"
														"#默认为1"
													;

		constexpr const char FontName[]				= u8"#信息字体名(非路径)。必须为已在系统中安装的字体。默认为黑体"
													;

		constexpr const char FontColor[]			= u8"#信息字体颜色。格式为0xAARRGGBB。默认为0xFFFFFFFF(白色)"
													;

		constexpr const char EnableKeys[]			= u8"#启用按键控制，按键配置如下：\n"
#ifdef ZA
														";   退格  切换OriginalVoice的值\n"
#endif
														";    +    Volume加1(若同时按住SHIFT则加5)\n"
														";    -    Volume减1(若同时按住SHIFT则减5)\n"
														";  -+同时 设置静音 (若在静音状态调整了Volume，静音状态会被取消)\n"
														";    0    切换AutoPlay的值\n"
														";    9    切换SkipVoice的值\n"
														";    8    切换DisableDialogSE的值\n"
														";    7    切换DisableDududu的值\n"
														";    6    切换ShowInfo的值\n"
														"; \\(按住) 显示当前全部配置项的设置(这里将无视ShowInfo的设置)\n"
														";  []同时 取消静音，同时：\n"
														";         1.当SaveChange为1时，将EnableKeys和SaveChange以外的所有配置项设为默认值；\n"
														";         2.当SaveChange为0时，重新加载配置项文件。(无配置文件时，同1)\n"
														"###注意：除\\键外，长按不论持续多长时间，均视为一次按键###\n"
														"#默认为1(启用)"
													;

		constexpr const char SaveChange[]			= u8"#若在游戏中修改了配置，是否保存更改。\n"
														"#这个配置项仅在启用了上一配置项后有意义\n"
														"#默认为1(保存)\n"
														"###注意：静音状态不会被保存###"
													;
	}
}



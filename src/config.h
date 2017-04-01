#pragma once

#define STR_Volume "Volume"
#define STR_AutoPlay "AutoPlay"
#define STR_SkipVoice "SkipVoice"
#define STR_DisableDialogSE "DisableDialogSE"
#define STR_DisableDududu "DisableDududu"

#define STR_EnableKeys "EnableKeys"
#define STR_SaveChange "SaveChange"

#define MAX_Volume 100

#define DFT_Volume			MAX_Volume
#define DFT_AutoPlay		1
#define DFT_SkipVoice		1
#define DFT_DisableDialogSE	1
#define DFT_DisableDududu	1

#define DFT_EnableKeys 1
#define DFT_SaveChange 0

struct Config
{
	int Volume;
	int AutoPlay;
	int SkipVoice;
	int DisableDialogSE;
	int DisableDududu;

	int EnableKeys;
	int SaveChange;

	bool LoadConfig(const char* configFn);
	bool SaveConfig(const char* configFn) const;

	void Reset(bool all = false) { load_default(all); }
	Config() { load_default(); }
private:
	void load_default(bool all = true);
};

#define CMT_Volume			"#音量，范围为0~100，默认为100(最大值)"
#define CMT_AutoPlay		"#自动播放，仅在有语音时有效。默认为1(开启)"
#define CMT_SkipVoice		"#对话框关闭时，终止语音。默认为1(开启)"
#define CMT_DisableDialogSE	"#在语音播放时，禁用对话框关闭(切换)时的音效。默认为1(开启)"
#define CMT_DisableDududu	"#在语音播放时，禁用文字显示的嘟嘟声。默认为1(开启)"

#define CMT_EnableKeys	"#启用按键控制，按键配置如下：\n"\
						";    +    Volume加1(若同时按住SHIFT则加5)\n"\
						";    -    Volume减1(若同时按住SHIFT则减5)\n"\
						";    0    静音/取消静音(若在静音状态调整了Volume，静音状态会被取消)\n"\
						";    9    切换AutoPlay的值\n"\
						";    8    切换SkipVoice的值\n"\
						";    7    切换DisableDialogSE的值\n"\
						";    6    切换DisableDududu的值\n"\
						";  -+同时 取消静音，同时：\n"\
						";         1.当SaveChange为1时，将EnableKeys和SaveChange以外的所有配置项到默认值；\n"\
						";         2.当SaveChange为0时，重新加载配置项文件。(无配置文件时，同1)\n"\
						"#注意：长按不论持续多长时间，均视为一次按键\n"\
						"#默认为1(启用)"

#define CMT_SaveChange	"#若在游戏中修改了配置，是否保存更改。\n"\
						"#这个配置项仅在启用了上一配置项后有意义\n"\
						"#默认为0(不保存)"



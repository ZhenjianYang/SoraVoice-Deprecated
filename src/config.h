#pragma once

#define STR_Volume "Volume"
#define STR_DisableDududu "DisableDududu"
#define STR_DisableDialogSE "DisableDialogSE"
#define STR_SkipVoice "SkipVoice"

#define MAX_Volume 100
#define DFT_Volume MAX_Volume

#define DFT_DisableDududu 1
#define DFT_DisableDialogSE 1
#define DFT_SkipVoice 1

struct Config
{
	int Volume;
	int DisableDududu;
	int DisableDialogSE;
	int SkipVoice;

	bool LoadConfig(const char* configFn);
	bool SaveConfig(const char* configFn) const;

	Config() { load_default(); }
private:
	void load_default();
};

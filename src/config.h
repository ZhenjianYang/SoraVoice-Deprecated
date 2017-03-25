#pragma once

#define STR_Volume "Volume"
#define STR_DisableDududu "DisableDududu"

#define MAX_Volume 100
#define DFT_Volume MAX_Volume

#define DFT_DisableDududu 1

struct Config
{
	int Volume;
	int DisableDududu;

	bool LoadConfig(const char* configFn);
	bool SaveConfig(const char* configFn) const;

	Config() { load_default(); }
private:
	void load_default();
};

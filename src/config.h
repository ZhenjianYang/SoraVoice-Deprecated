#pragma once

#define STR_Volume "Volume"

#define MAX_Volume 100
#define DFT_Volume MAX_Volume

struct Config
{
	int Volume;

	bool LoadConfig(const char* configFn);
	bool SaveConfig(const char* configFn) const;

	Config() { load_default(); }
private:
	void load_default();
};

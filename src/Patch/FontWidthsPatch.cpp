#include "FontWidthsPatch.h"

#include <Windows.h>

#include <fstream>
#include <string>

static constexpr char FN_FONT_WIDTH[] = "voice/fonts/FONTWDTH._DA";
static constexpr int MIN_ANSI = 0x20;
static constexpr int MAX_ANSI = 0x7F;
static constexpr int MIN_WIDTH = 2;
//static constexpr int MAX_WIDTH = 18;

bool FontWidthsPatch::Apply(void * p_fontwidths) {
	if (!p_fontwidths) return false;

	std::ifstream ifs(FN_FONT_WIDTH, std::ios::in);
	if (!ifs) return false;

	DWORD dwProtect;
	if (!VirtualProtect(p_fontwidths, sizeof(int) * (MAX_ANSI + 1), PAGE_READWRITE, &dwProtect)) {
		return false;
	}

	int* fontwidths = (int*)p_fontwidths;
	std::string s;
	while (std::getline(ifs, s)) {
		if (s.empty() || s[0] == '#') continue;
		int ansi, width;
		if (2 != std::sscanf(s.c_str(), "%X %d", &ansi, &width)) continue;
		if (ansi < MIN_ANSI || ansi > MAX_ANSI) continue;
		if (width < MIN_WIDTH) continue;
		fontwidths[ansi] = width - MIN_WIDTH;
	}
	ifs.close();

	DWORD dwProtect2;
	VirtualProtect(p_fontwidths, sizeof(int) * (MAX_ANSI - MIN_ANSI + 1), dwProtect, &dwProtect2);

	return true;
}

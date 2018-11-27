#pragma once

namespace DrawTexts {
	bool Init();
	bool End();

	void* DrawTexts(const char* text, void* buffer, unsigned stride, unsigned color_index);
};

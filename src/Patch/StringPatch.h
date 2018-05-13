#pragma once

namespace StringPatch {

	bool LoadStrings(const char* fileName);

	void SetPattern(const char* pattern);

	int Apply(void* start, int size, const char* pattern = nullptr);
}

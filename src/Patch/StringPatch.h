#pragma once

namespace StringPatch {
	using EditFun = void(void*, int);

	bool LoadStrings(const char* fileName);

	void SetPattern(const char* pattern);

	int Apply(void* start, int size, const char* pattern = nullptr);

	void SetEditFun(EditFun* editFun);
}

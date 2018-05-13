#pragma once

namespace StringPatch {

	bool LoadStrings(const char* fileNmae);

	bool Apply(void* start, int size, const char* pattern);
}

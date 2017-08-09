#pragma once

#include "ed_voice.h"

using EDVoice = struct EDVoice {
	void* ip;
	bool start;
	decltype(::Init)* Init;
	decltype(::End)* End;
};

bool InitEDVoice(void* hDll, EDVoice* sv);


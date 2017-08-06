#pragma once

#include "rc_ed_voice\resource.h"

#define RC_DataType "ED_VOICE"

#define RC_TABLE { \
	{ "voice/Message_en.ini", IDR_MESSAGE_EN, RC_DataType}, \
	{ "voice/Message_zh.ini", IDR_MESSAGE_ZH, RC_DataType}, \
	{ nullptr, 0, nullptr},  \
}


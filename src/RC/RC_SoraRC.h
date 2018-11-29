#pragma once

#include "resource.h"

#define RC_DataType "ED_VOICE"

#define RC_TABLE { \
	{ "voice/Message_en.ini", IDR_MESSAGE_EN, RC_DataType}, \
	{ "voice/Message_zh.ini", IDR_MESSAGE_ZH, RC_DataType}, \
	{ "voice/SoraData.ini", IDR_SORADATA, RC_DataType}, \
	{ "voice/SoraDataSora.ini", IDR_SORADATA_SORA, RC_DataType}, \
	{ "voice/SoraDataTITS.ini", IDR_SORADATA_TITS, RC_DataType}, \
	{ "voice/SoraDataZA.ini", IDR_SORADATA_ZA, RC_DataType}, \
	{ "voice/ao_rnd_vlst.txt", IDR_AO_RND_VLST, RC_DataType}, \
	{ nullptr, 0, nullptr},  \
}


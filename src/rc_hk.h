#pragma once

#include "rc_hk\resource.h"

#define RC_DataType "SORADATA"

#define RC_TABLE { \
	{ "voice/SoraData.ini", IDR_SORADATA, RC_DataType}, \
	{ "voice/sora_all", IDR_SORA_BIN, RC_DataType}, \
	{ "voice/za_all", IDR_ZA_BIN, RC_DataType}, \
	{ "voice/tits_all", IDR_TITS_BIN, RC_DataType}, \
	{ "voice/ao_rnd_vlst.txt", IDR_AO_RND_VLST, RC_DataType}, \
	{ nullptr, 0, nullptr},  \
}


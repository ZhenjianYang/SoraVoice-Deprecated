#pragma once

#include <dsound.h>
#include <vorbis\vorbisfile.h>

using byte = unsigned char;

using VF_ov_open_callbacks = decltype(ov_open_callbacks);
using VF_ov_info = decltype(ov_info);
using VF_ov_read = decltype(ov_read);
using VF_ov_clear = decltype(ov_clear);

struct VF {
	VF_ov_open_callbacks* ov_open_callbacks;
	VF_ov_info* ov_info;
	VF_ov_read* ov_read;
	VF_ov_clear* ov_clear;
};

struct InitParam
{
	int IsAo;
	struct {
		byte Playing;
		byte DisableDududu;
		byte DisableDialogSE;
		byte Reversed;
	} Flags;
	int Reversed1;
	int Reversed2;

	LPDIRECTSOUND* p_pDS;

	VF_ov_open_callbacks** p_ov_open_callbacks;
	VF_ov_info** p_ov_info;
	VF_ov_read** p_ov_read;
	VF_ov_clear** p_ov_clear;
};


#pragma once

#include <dsound.h>
#include <dinput.h>
#include <vorbis\vorbisfile.h>

using byte = unsigned char;

using VF_ov_open_callbacks = decltype(ov_open_callbacks);
using VF_ov_info = decltype(ov_info);
using VF_ov_read = decltype(ov_read);
using VF_ov_clear = decltype(ov_clear);

struct SVData;

#pragma pack(push, 1)
struct InitParam
{
	//0x00
	SVData* sv;

	//0x04
	byte isAo;
	struct Status {
		byte ended;
		byte playing;
		byte reversedStatus1;
	} status;

	struct Order {
	//0x08
		byte disableDududu;
		byte disableDialogSE;
		byte skipVoice;
		byte autoPlay;
	} order;

	int reversed1;

	//0x10
	VF_ov_open_callbacks** p_ov_open_callbacks;
	VF_ov_info** p_ov_info;
	VF_ov_read** p_ov_read;
	VF_ov_clear** p_ov_clear;

	//0x20
	char* p_Keys;
	LPDIRECTSOUND* p_pDS;
};
#pragma pack(pop)




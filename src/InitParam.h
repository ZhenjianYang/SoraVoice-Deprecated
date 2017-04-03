#pragma once

typedef unsigned char byte;

#pragma pack(push, 1)
struct InitParam
{
	//0x00
	void* sv;

	//0x04
	struct Status {
		byte ended;
		byte playing;
		byte mute;
		byte showing;
	} status;

	struct Order {
	//0x08
		byte disableDududu;
		byte disableDialogSE;
		byte skipVoice;
		byte autoPlay;
	} order;

	//0x0C
	void* pHwnd;

	//0x10
	void** p_ov_open_callbacks;
	void** p_ov_info;
	void** p_ov_read;
	void** p_ov_clear;

	//0x20
	char* p_Keys;
	void** p_pDS;
	//0x28
	char keysOld[8];
};
#pragma pack(pop)




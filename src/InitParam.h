#pragma once

typedef unsigned char byte;

#pragma pack(push, 1)
struct InitParam
{
	//0x00
	void* sv;
	unsigned reserved;

	//0x08
	struct Status {
		byte ended;
		byte playing;
		byte mute;
		byte showing;

	//0x0C
		byte wait;
		byte waitv;
		byte code5;
		byte reserved_status[1];
	} status;

	//0x10
	struct Order {
		byte disableDududu;
		byte disableDialogSE;
		byte skipVoice;
		byte autoPlay;

	//0x14
		byte reserved_order[4];
	} order;

	//0x18
	void** p_ov_open_callbacks;
	//0x1C
	void** p_ov_info;
	//0x20
	void** p_ov_read;
	//0x24
	void** p_ov_clear;
	//0x28
	void** reserved_api[2];

	//0x30
	void** reserved_ptr[4];
	//0x40
	void** p_d3dd;
	//0x44
	void* p_Hwnd;
	//0x48
	void** p_pDS;
	//0x4C
	char* p_Keys;

	//0x50
	char keysOld[8];
	//0x58
	unsigned recent;
	//0x5C
	unsigned now;
	//0x60
	unsigned time_textbeg;
	//0x64
	unsigned count_ch;
};
#pragma pack(pop)




#pragma once

typedef unsigned char byte;

#pragma pack(push, 1)
struct InitParam
{
	//0x00
	struct {
		//0x00
		void** p_ov_open_callbacks;
		//0x04
		void** p_ov_info;
		//0x08
		void** p_ov_read;
		//0x1C
		void** p_ov_clear;

		//0x10
		void** p_d3dd;
		//0x14
		void** p_did;
		//0x18
		void** p_Hwnd;
		//0x1C
		void** p_pDS;

		//0x20
		void*  p_mute;
		//0x24
		const char* p_keys;
		//0x28
		void** reserved[2];
	} addrs;

	//0x30
	unsigned exps[8];

	//0x50
	struct {
		unsigned next;
		unsigned to;
	} jcs[7];

	//0x88
	byte scodes[8];

	//0x90
	struct {
		//0x90
		unsigned recent;
		//0x94
		unsigned now;
		//0x98
		unsigned time_textbeg;
		//0x9C
		unsigned count_ch;
	} rcd;
	
	//0xA0
	void* sv;

	//0xA4
	struct Status {
	//0xA4
		byte ended;
		byte playing;
		byte mute;
		byte showing;

	//0xA8
		byte wait;
		byte waitv;
		byte code5;
		byte scode;

	//0xAC
		byte reserved[4];
	} status;

	//0xB0
	struct Order {
		byte disableDududu;
		byte disableDialogSE;
		byte skipVoice;
		byte autoPlay;

	//0xB4
		byte reserved[4];
	} order;

	//0xB8
	char Comment[40];
};
#pragma pack(pop)

static_assert(sizeof(void*) == 4, "32 bits only!");
static_assert(sizeof(InitParam) <= 0xE0, "Size of InitParam too big!");


#pragma once

#include "Type.h"

#pragma pack(push, 1)
struct InitParam
{
	//0x00
	struct {
		//0x00
		void** p_d3dd;
		//0x04
		void** p_did;
		//0x08
		void** p_Hwnd;
		//0x0C
		void** p_pDS;

		//0x10
		void*  p_mute;
		//0x14
		const u8* p_keys;
		//0x18
		void** p_global;

		//0x1C
		void** reserved[4];
	} addrs;

	//0x2C
	char *p_rnd_vlst;

	//0x30
	u32 exps[8];

	//0x50
	struct {
		u32 next;
		u32 to;
	} jcs[7];

	//0x88
	u8 scodes[8];

	//0x90
	struct {
		//0x90
		u32 recent;
		//0x94
		u32 now;
		//0x98
		u32 time_textbeg;
		//0x9C
		u32 count_ch;
	} rcd;
	
	//0xA0
	void* sv;

	//0xA4
	struct Status {
	//0xA4
		u8 ended;
		u8 playing;
		u8 mute;
		u8 showing;

	//0xA8
		u8 wait;
		u8 waitv;
		u8 scode;

	//0xAB
		u8 reserved[5];
	} status;

	//0xB0
	struct Order {
		u8 disableDududu;
		u8 disableDialogSE;
		u8 autoPlay;

	//0xB3
		u8 reserved[5];
	} order;

	//0xB8
	char Comment[40];
};
#pragma pack(pop)

static_assert(sizeof(void*) == 4, "32 bits only!");
static_assert(sizeof(InitParam) <= 0xE0, "Size of InitParam too big!");

bool InitAddrs(InitParam* initParam);

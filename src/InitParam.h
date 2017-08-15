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
		void* addr_ppscn;

		//0x20
		void* addr_iscn;
		//0x24
		void** reserved[6];
	} addrs;

	//0x3C
	char *p_rnd_vlst;

	//0x40
	u32 exps[16];

	//0x80
	struct {
		u32 next;
		u32 to;
	} jcs[16];

	//0x100
	u8 scodes[16];

	//0x110
	struct {
		//0x110
		u32 recent;
		//0x114
		u32 now;
		//0x118
		u32 time_textbeg;
		//0x11C
		u32 count_ch;
	} rcd;
	
	//0x120
	void* sv;

	//0x124
	struct Status {
	//0x124
		u8 ended;
		u8 playing;
		u8 mute;
		u8 showing;

	//0x128
		u8 wait;
		u8 waitv;
		u8 scode;
		u8 playingOri;
	//0x12C
		u8 reserved[8];
	} status;

	//0x134
	struct Order {
		u8 disableDududu;
		u8 disableDialogSE;
		u8 autoPlay;

	//0x137
		u8 reserved[9];
	} order;

	//0x140
	char Comment[64];
};
#pragma pack(pop)

static_assert(sizeof(void*) == 4, "32 bits only!");
static_assert(sizeof(InitParam) <= 0x1C0, "Size of InitParam too big!");

bool InitAddrs(InitParam* initParam, void* hDll);

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct SVData
{
	typedef unsigned char byte;
	typedef struct Jcs {
		unsigned next;
		unsigned to;
	} Jcs;
	enum GAMES {
		INVALID_GAME = 0,
		SORA_FC = 1,
		SORA_SC = 2,
		SORA_3RD = 3,
		ZERO = 4,
		AO = 5,
	};
	enum SERIES {
		SERIES_NONE = 0,
		SERIES_SORA = 1,
		SERIES_ZEROAO = 2,
		SERIES_TITS = 3,
	};
	enum DX_VER {
		DXDFT = 0,
		DX8 = 1,
		DX9 = 2
	};

	int game;
	int series;
	int dxver;

#define SERIES_IS_ED6(series) ((series) == SVData::SERIES_SORA  || (series) == SVData::SERIES_TITS)

	struct Status {
		unsigned startup;
		unsigned ended;
		unsigned playing;
		unsigned mute;

		unsigned showing;
		unsigned wait;
		unsigned waitv;
		unsigned scode;

		unsigned playingOri;
		unsigned first_text;
	} status;

	struct Order {
		unsigned disableDududu;
		unsigned disableDialogSE;
		unsigned autoPlay;
	} order;

	struct {
		unsigned recent;
		unsigned now;
		unsigned time_textbeg;
		unsigned count_ch;
	} rcd;

	struct {
		void** p_d3dd;
		void** p_did;
		void** p_Hwnd;
		void** p_pDS;
		void** p_global;
		const byte* p_keys;
		void* p_mute;

		void* addr_ppscn;
		void* addr_iscn;
	} addrs;

	struct {
		Jcs text;
		Jcs dududu;
		Jcs dlgse;
		Jcs aup;
		Jcs scode;
		Jcs ldscn;
		Jcs ldscnB;
		Jcs ldscnB2;
		Jcs scnp;
	} jcs;

	struct Scode {
		unsigned TEXT;
		unsigned SAY;
		unsigned TALK;
		unsigned MENU;
		unsigned MENUEND;
	} scode;

	const char *p_rnd_vlst;
	char Comment[64];
};

int InitSVData();

extern SVData SV;

#ifdef __cplusplus
}
#endif

static_assert(sizeof(void*) == 4, "32 bits only!");

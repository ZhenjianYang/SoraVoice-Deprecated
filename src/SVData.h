#pragma once

#include <SVEnum.h>

#ifdef __cplusplus
extern "C" {
#endif

struct SVData
{
	typedef struct Jcs {
		unsigned next;
		unsigned to;
	} Jcs;

	int game;
	int series;
	int dxver;

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
		unsigned time_textend;
		unsigned count_ch;
	} rcd;

	struct {
		void** p_d3dd;
		void** p_did;
		void** p_Hwnd;
		void** p_pDS;
		void** p_global;
		const char* p_keys;
		void* p_mute;

		void* addr_ppscn;
		void* addr_iscn;
		void* addr_quizp;
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
		Jcs ldquiz;
		Jcs ldquizB;
		Jcs scnp;
		Jcs prst;
		Jcs prst2;
		Jcs prst3;
		Jcs prst4;
		Jcs prst5;
	} jcs;

	struct Scode {
		unsigned TEXT;
		unsigned SAY;
		unsigned TALK;
		unsigned MENU;
		unsigned MENUEND;
		unsigned MINIGAME;
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

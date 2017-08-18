#pragma once

#define GAME_IS_SORA(game) ((game) == SVData::SORA)
#define GAME_IS_TITS(game) ((game) == SVData::TITS_DX8 || (game) == SVData::TITS_DX9)
#define GAME_IS_ED6(game) (GAME_IS_SORA(game) || GAME_IS_TITS(game))
#define GAME_IS_ZA(game) ((game) == SVData::ZERO || (game) == SVData::AO)
#define GAME_IS_DX8(game) ((game) == SVData::SORA || (game) == SVData::TITS_DX8)
#define GAME_IS_DX9(game) ((game) == SVData::TITS_DX9 || (game) == SVData::ZERO || (game) == SVData::AO)
#define GAME_IS_VALID(game) (GAME_IS_ED6(game) || GAME_IS_ZA(game))

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
	typedef enum Game {
		NONE = 0,
		SORA = 1,
		TITS_DX8 = 2,
		TITS_DX9 = 3,
		ZERO = 11,
		AO = 12,
	} Game;

	Game game;
	int sora;
	int tits;
	int za;

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
int CleanSVData();

extern SVData SV;

#ifdef __cplusplus
}
#endif

static_assert(sizeof(void*) == 4, "32 bits only!");

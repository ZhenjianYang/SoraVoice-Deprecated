#pragma once

#include "Type.h"

#define GAME_IS_SORA(game) ((game) == InitParam::SORA || (game) == InitParam::TITS_DX8 || (game) == InitParam::TITS_DX9)
#define GAME_IS_TITS(game) ((game) == InitParam::TITS_DX8 || (game) == InitParam::TITS_DX9)
#define GAME_IS_ZA(game) ((game) == InitParam::ZERO || (game) == InitParam::AO)
#define GAME_IS_DX8(game) ((game) == InitParam::SORA || (game) == InitParam::TITS_DX8)
#define GAME_IS_DX9(game) ((game) == InitParam::TITS_DX9 || (game) == InitParam::ZERO || (game) == InitParam::AO)
#define GAME_IS_VALID(game) (GAME_IS_SORA(game) || GAME_IS_ZA(game))

struct InitParam
{
	typedef unsigned char byte;
	typedef struct Jcs {
		unsigned next;
		unsigned to;
	} Jcs;
	typedef enum Game {
		SORA = 1,
		TITS_DX8 = 2,
		TITS_DX9 = 3,
		ZERO = 11,
		AO = 12,
	} Game;

	Game game;
	int dx9;
	void* sv;

	struct Status {
		unsigned ended;
		unsigned playing;
		unsigned mute;
		unsigned showing;

		unsigned wait;
		unsigned waitv;
		unsigned scode;
		unsigned playingOri;
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

		void* addr_mute;
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
		Jcs scnp;
	} jcs;

	struct {
		unsigned TEXT;
		unsigned SAY;
		unsigned TALK;
		unsigned MENU;
		unsigned MENUEND;
	} scode;

	char *p_rnd_vlst;
	char Comment[64];
};
static_assert(sizeof(void*) == 4, "32 bits only!");

constexpr int size = sizeof(InitParam);

bool InitAddrs(InitParam* initParam, void* hDll);

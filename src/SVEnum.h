#pragma once

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

#define SERIES_IS_ED6(series) ((series) == SERIES_SORA  || (series) == SERIES_TITS)

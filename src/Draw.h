#pragma once

#include "Type.h"

class Draw {
public:
	enum class InfoType
	{
		Hello = 0,
		InfoOnoff,
		AutoPlayMark,

		Volume,
		OriginalVoice,
		AutoPlay,
		SkipVoice,
		DisableDialogSE,
		DisableDududu,

		ConfigReset,

		All,
		Dead
	};

	static constexpr unsigned ShowTimeInfinity = 0;

	static Draw * CreateDraw(u8& showing, void * hWnd, void * pD3DD, const char* fontName);
	static void DestoryDraw(Draw * draw);

	virtual void DrawInfos() = 0;

	virtual void AddInfo(InfoType type, unsigned time, unsigned color, const char* text, ...) = 0;

	virtual void RemoveInfo(InfoType type) = 0;

	const u8& Showing() const { return showing; }

protected:
	Draw(u8& showing) : showing(showing) { }
	u8 &showing;
	virtual ~Draw() { };
};


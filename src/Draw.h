#pragma once

class Draw {
public:
	enum class InfoType
	{
		Hello,
		InfoOnoff,
		AutoPlayMark,

		Volume,
		AutoPlay,
		SkipVoice,
		DisableDialogSE,
		DisableDududu,

		ConfigReset,

		All,
		Dead
	};

	static constexpr unsigned ShowTimeInfinity = 0;

	static Draw * CreateDraw(char& showing, void * hWnd, void * pD3DD, void* p_D3DXCreateFontIndirect, const char* fontName);
	static void DestoryDraw(Draw * draw);

	virtual void DrawInfos() = 0;

	virtual void AddInfo(InfoType type, unsigned time, unsigned color, const char* text, ...) = 0;

	virtual void RemoveInfo(InfoType type) = 0;

	const char& Showing() const { return showing; }

protected:
	Draw(char& showing) : showing(showing) { }
	char &showing;
	virtual ~Draw() { };
};


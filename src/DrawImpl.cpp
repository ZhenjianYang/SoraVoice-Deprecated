#include "Draw.h"

#include "Log.h"
#include "Clock.h"

#ifdef ZA
#include <d3dx9.h>
#else
#include <d3d8/d3dx8.h>
#endif

#if DIRECT3D_VERSION == 0x0900
#define D3D IDirect3D9
#define D3DD IDirect3DDevice9
#define SPRITE NULL,
#elif DIRECT3D_VERSION == 0x0800
#define D3D IDirect3D8
#define D3DD IDirect3DDevice8
#define SPRITE
#else
static_assert(DIRECT3D_VERSION == 0x0900 || DIRECT3D_VERSION == 0x0800,
	"DIRECT3D_VERSION must be 0x0900 or 0x0800")
#endif

#include <list>
#include <set>
#include <memory>

constexpr int MAX_TEXT_LEN = 63;

constexpr int MIN_FONT_SIZE = 25;
constexpr int TEXT_NUM_SCRH = 25;

constexpr double BOUND_WIDTH_RATE = 0.5;
constexpr double LINE_SPACE_RATE = 0.15;
constexpr double TEXT_SHADOW_POS_RATE = 0.08;

constexpr unsigned SHADOW_COLOR = 0x40404040;

constexpr unsigned TIME_MAX = 0xFFFFFFFF;

class DrawImpl : private Draw 
{
	friend class Draw;

	virtual void DrawInfos() override;

	virtual void AddInfo(InfoType type, unsigned color, unsigned time, const char* text, ...) override;

	virtual void RemoveInfo(InfoType type) override {
		switch (type)
		{
		case InfoType::All:
			infos.clear();
			break;
		case InfoType::Dead:
			infos.remove_if([](const PtrInfo& t) { return t->deadTime < Clock::Now(); });
			break;
		default:
			infos.remove_if([&type](const PtrInfo& t) { return t->type == type; });
			break;
		}
		showing = infos.size() > 0;
	}

	DrawImpl(char& showing, void* hWnd, void* pD3DD, void* p_D3DXCreateFontIndirect, const char* fontName)
		:Draw(showing),
		hWnd((HWND)hWnd), pD3DD((decltype(this->pD3DD))pD3DD), pD3DXCreateFontIndirect((CallCreateFont)p_D3DXCreateFontIndirect)
	{
		constexpr int len = sizeof(desca.FaceName);
		for (int i = 0; i < len && fontName[i]; i++) {
			desca.FaceName[i] = fontName[i];
		}
		desca.FaceName[len - 1] = '\0';

		desca.Height = -MIN_FONT_SIZE;
		desca.Weight = FW_NORMAL;
		desca.CharSet = DEFAULT_CHARSET;
		desca.OutputPrecision = OUT_OUTLINE_PRECIS;
		desca.Quality = CLEARTYPE_QUALITY;

		RECT rect;
		if (GetClientRect(this->hWnd, &rect)) {
			width = rect.right - rect.left;
			height = rect.bottom - rect.top;

			int fontSize = height / TEXT_NUM_SCRH;
			if (fontSize < MIN_FONT_SIZE) fontSize = MIN_FONT_SIZE;
			desca.Height = -fontSize;

			bound = (int)(fontSize * BOUND_WIDTH_RATE + 0.5);
			shadow = (int)(fontSize * TEXT_SHADOW_POS_RATE + 0.5);
			linespace = (int)(fontSize * LINE_SPACE_RATE + 0.5);

			LOG("screen width = %d", width);
			LOG("screen height = %d", height);
			LOG("Font Size = %d", fontSize);
			LOG("Font Name = %s", desca.FaceName);
		}
	}

	~DrawImpl() {
		if (pFont) pFont->Release();
	}

	struct Info
	{
		char text[MAX_TEXT_LEN + 1];
		RECT rect;
		unsigned color;
		unsigned format;
		InfoType type;

		unsigned deadTime;
	};
	using PtrInfo = std::unique_ptr<Info>;
	using PtrInfoList = std::list<PtrInfo>;
	using CallCreateFont = decltype(D3DXCreateFontIndirect)*;

#if DIRECT3D_VERSION == 0x900
	D3DXFONT_DESCA desca;
#else
	LOGFONT lf;
	struct _DESCA {
		LONG &Height;
		LONG &Width;
		LONG &Weight;
		UINT MipLevels;
		BYTE &Italic;
		BYTE &CharSet;
		BYTE &OutputPrecision;
		BYTE &Quality;
		BYTE &PitchAndFamily;
		CHAR(&FaceName)[LF_FACESIZE];

		_DESCA(LOGFONT& lf) :
			Height(lf.lfHeight),
			Width(lf.lfWidth),
			Weight(lf.lfWeight),
			MipLevels(0),
			Italic(lf.lfItalic),
			CharSet(lf.lfCharSet),
			OutputPrecision(lf.lfOutPrecision),
			Quality(lf.lfQuality),
			PitchAndFamily(lf.lfPitchAndFamily),
			FaceName(lf.lfFaceName) { }
	} desca {lf};
#endif // DIRECT3D_VERSION == 0x900

	const HWND hWnd;
	D3DD* const pD3DD;
	CallCreateFont pD3DXCreateFontIndirect;

	ID3DXFont *pFont = nullptr;

	int width = 0;
	int height = 0;
	int bound = 0;
	int linespace = 0;
	int shadow = 0;
	
	PtrInfoList infos;
};

Draw * Draw::CreateDraw(char& showing, void * hWnd, void * pD3DD, void* p_D3DXCreateFontIndirect, const char* fontName)
{
	DrawImpl* draw = new DrawImpl(showing, hWnd, pD3DD, p_D3DXCreateFontIndirect, fontName);
	return draw;
}

void Draw::DestoryDraw(Draw * draw)
{
	delete draw;
}

void DrawImpl::DrawInfos() {
	if (width == 0 || !pD3DD) return;

	pD3DD->BeginScene();
	//D3DXFONT_DESCA desca;

	if (!pFont && pD3DXCreateFontIndirect) {
#if DIRECT3D_VERSION == 0x900
		pD3DXCreateFontIndirect(pD3DD, &desca, &pFont);
#else
		pD3DXCreateFontIndirect(pD3DD, &lf, &pFont);
#endif	
	}

	if (pFont) {
		RECT rect_shadow;

		for (const auto& info : infos) {
			rect_shadow = info->rect;
			if (info->format & DT_RIGHT) rect_shadow.right += shadow;
			else rect_shadow.left += shadow;
			if (info->format & DT_BOTTOM) rect_shadow.bottom += shadow;
			else rect_shadow.top += shadow;

			unsigned color_shadow = (0xFFFFFF & SHADOW_COLOR) | (((info->color >> 24) * 3 / 4) << 24);

			pFont->DrawTextA(SPRITE info->text, -1, &rect_shadow, info->format, color_shadow);
		}

		for (const auto& info : infos) {
			pFont->DrawTextA(SPRITE info->text, -1, &info->rect, info->format, info->color);
		}
	}

	pD3DD->EndScene();
	//if (font) {
	//	font->Release(); font = NULL;
	//}
}

void DrawImpl::AddInfo(InfoType type, unsigned time, unsigned color, const char* text, ...) {
	unsigned dead = time == ShowTimeInfinity ? TIME_MAX : Clock::Now() + time;

	LOG("Add text, type = %d", type);
	const int h = desca.Height < 0 ? -desca.Height : desca.Height;

	auto it = infos.end();

	if (type != InfoType::Hello) {
		for (it = infos.begin(); it != infos.end(); ++it) {
			if ((*it)->type == type) {
				LOG("No need to Create new Text.");
				break;
			}
		}
	}

	if (it == infos.end()) {
		LOG("Create new Text.");
		infos.push_back(PtrInfo(new Info));
		it = --infos.end();
	}
	(*it)->type = type;
	(*it)->color = color;
	(*it)->deadTime = dead;
	(*it)->format = type == InfoType::AutoPlayMark ? DT_BOTTOM | DT_LEFT : DT_TOP | DT_LEFT;

	va_list argptr;
	va_start(argptr, text);
	vsnprintf((*it)->text, sizeof((*it)->text), text, argptr);
	va_end(argptr);
	int text_width = (int)(strlen((*it)->text) * h * 0.6);
	LOG("Text is %s", (*it)->text);
	LOG("Text width is %d", text_width);

	auto& rect = (*it)->rect;
	memset(&rect, 0, sizeof(rect));

	std::set<int> invalid_bottom, invalid_top;
	for (auto it2 = infos.begin(); it2 != infos.end(); ++it2) {
		if (it == it2 || (((*it)->format & DT_RIGHT) != ((*it2)->format & DT_RIGHT))) continue;
		invalid_top.insert((*it2)->rect.top);
		invalid_bottom.insert((*it2)->rect.bottom);
	}

	if ((*it)->format & DT_BOTTOM) {
		for (int bottom = height - bound; ; bottom -= h + bound) {
			if (invalid_bottom.find(bottom) == invalid_bottom.end()) {
				rect.bottom = bottom;
				break;
			}
		}
		rect.top = rect.bottom - h - linespace;
	}
	else {
		for (int top = bound; ; top += h + linespace) {
			if (invalid_top.find(top) == invalid_top.end()) {
				rect.top = top;
				break;
			}
		}

		rect.bottom = rect.top + h + linespace;
	}

	if ((*it)->format & DT_RIGHT) {
		rect.right = width - bound;
		rect.left = rect.right - text_width;
	}
	else {
		rect.left = bound;
		rect.right = rect.left + text_width;
	}

	LOG("top = %d, bottom = %d, left = %d, right = %d", rect.top, rect.bottom, rect.left, rect.right);

	showing = infos.size() > 0;
}


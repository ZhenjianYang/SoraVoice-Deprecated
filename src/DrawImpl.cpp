#include "Draw.h"

#include "Log.h"
#include "Clock.h"
#include "ApiPack.h"
#include "EncodeHelper.h"

#ifdef ZA
#include <d3d9/d3dx9.h>
#else
#include <d3d8/d3dx8.h>
#endif

#if DIRECT3D_VERSION == 0x0900
#define D3D IDirect3D9
#define D3DD IDirect3DDevice9
#elif DIRECT3D_VERSION == 0x0800
#define D3D IDirect3D8
#define D3DD IDirect3DDevice8
#else
static_assert(DIRECT3D_VERSION == 0x0900 || DIRECT3D_VERSION == 0x0800,
	"DIRECT3D_VERSION must be 0x0900 or 0x0800")
#endif

#if DIRECT3D_VERSION == 0x0900
#define SPRITE_BEGIN_FLAG D3DXSPRITE_ALPHABLEND
#define SPRITE pSprite,
#else
#define SPRITE_BEGIN_FLAG
#define SPRITE
#endif

#include <list>
#include <set>
#include <memory>

constexpr int MAX_TEXT_LEN = 63;

constexpr int MIN_FONT_SIZE = 20;
constexpr int TEXT_NUM_SCRH = 25;

constexpr double VBOUND_RATE = 0.3;
constexpr double HBOUND_RATE = 0.5;
constexpr double LINE_SPACE_RATE = 0.15;
constexpr double TEXT_SHADOW_POS_RATE = 0.08;

constexpr unsigned SHADOW_COLOR = 0x20202020;

constexpr unsigned TIME_MAX = 0xFFFFFFFF;

constexpr char STR_D3DXCreateFontIndirect[] = "D3DXCreateFontIndirect";
constexpr char STR_D3DXCreateSprite[] = "D3DXCreateSprite";

class DrawImpl : private Draw 
{
	friend class Draw;

	virtual void DrawInfos() override;

	virtual void AddInfoText(InfoType type, unsigned time, unsigned color, const char* text) override;

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

	DrawImpl(u8& showing, void* hWnd, void* pD3DD, const char* fontName)
		:Draw(showing),
		hWnd((HWND)hWnd), pD3DD((decltype(this->pD3DD))pD3DD), 
		pD3DXCreateFontIndirect((CallCreateFont)ApiPack::GetApi(STR_D3DXCreateFontIndirect)),
		pD3DXCreateSprite((CallCreateSprite)ApiPack::GetApi(STR_D3DXCreateSprite))
	{
#if DIRECT3D_VERSION == 0x900
		ConvertUtf8toUtf16(desca.FaceName, fontName);
#else
		constexpr int lfFaceName_len = std::extent<decltype(lf.lfFaceName)>::value;
		wchar buff[lfFaceName_len];
		ConvertUtf8toUtf16(buff, fontName);
		WideCharToMultiByte(CP_OEMCP, 0, buff, -1, desca.FaceName, sizeof(desca.FaceName), 0, 0);
#endif // DIRECT3D_VERSION == 0x900
		desca.Height = -MIN_FONT_SIZE;
		desca.Weight = FW_NORMAL;
		desca.CharSet = DEFAULT_CHARSET;
		desca.OutputPrecision = OUT_OUTLINE_PRECIS;
		desca.Quality = CLEARTYPE_QUALITY;

		RECT rect;
		if (GetClientRect(this->hWnd, &rect)) {
			width = rect.right - rect.left;
			height = rect.bottom - rect.top;

#ifdef ZA
			vfix = (height - width * 9 / 16) / 2;
#endif // ZA

			int fontSize = (height - 2 * vfix) / TEXT_NUM_SCRH;
			if (fontSize < MIN_FONT_SIZE) fontSize = MIN_FONT_SIZE;
			desca.Height = -fontSize;

			vbound = (int)(fontSize * VBOUND_RATE + 0.5);
			hbound = (int)(fontSize * HBOUND_RATE + 0.5);


			shadow = (int)(fontSize * TEXT_SHADOW_POS_RATE + 0.5);
			linespace = (int)(fontSize * LINE_SPACE_RATE + 0.5);

			LOG("screen width = %d", width);
			LOG("screen height = %d", height);
			LOG("Font Size = %d", fontSize);
			LOG("Font Name = %s", desca.FaceName);

			if (pD3DXCreateFontIndirect) {
#if DIRECT3D_VERSION == 0x900
				pD3DXCreateFontIndirect(this->pD3DD, &desca, &pFont);
#else
				pD3DXCreateFontIndirect(this->pD3DD, &lf, &pFont);
#endif
			}

			if (pD3DXCreateSprite) {
				pD3DXCreateSprite(this->pD3DD, &pSprite);
			}
		}
	}

	~DrawImpl() {
		if (pFont) pFont->Release();
		if (pSprite) pSprite->Release();
	}

	using wchar = wchar_t;

	struct Info
	{
		wchar text[MAX_TEXT_LEN + 1];
		RECT rect;
		unsigned color;
		unsigned format;
		InfoType type;

		unsigned deadTime;
	};
	using PtrInfo = std::unique_ptr<Info>;
	using PtrInfoList = std::list<PtrInfo>;
	using CallCreateSprite = decltype(D3DXCreateSprite)*;

#if DIRECT3D_VERSION == 0x900
	using CallCreateFont = decltype(D3DXCreateFontIndirectW)*;
#else
	using CallCreateFont = decltype(D3DXCreateFontIndirect)*;
#endif // DIRECT3D_VERSION == 0x900

	static constexpr const unsigned DftFormatList[] = {
			DT_TOP | DT_LEFT   ,//Hello = 0,
			DT_TOP | DT_LEFT   ,//InfoOnoff,
	#ifdef ZA
			DT_BOTTOM | DT_RIGHT,//AutoPlayMark,
	#else
			DT_BOTTOM | DT_LEFT,//AutoPlayMark,
	#endif
			DT_TOP | DT_LEFT   ,//Volume,
			DT_TOP | DT_LEFT   ,//OriginalVoice,
			DT_TOP | DT_LEFT   ,//OriVolumePercent,
			DT_TOP | DT_LEFT   ,//AutoPlay,
			DT_TOP | DT_LEFT   ,//SkipVoice,
			DT_TOP | DT_LEFT   ,//DisableDialogSE,
			DT_TOP | DT_LEFT   ,//DisableDududu,

			DT_TOP | DT_LEFT   ,//ConfigReset,

			DT_TOP | DT_LEFT   ,//All,
		};

#if DIRECT3D_VERSION == 0x900
	D3DXFONT_DESCW desca;
#else
	LOGFONTA lf;
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
		CHAR (&FaceName)[LF_FACESIZE];

		_DESCA(LOGFONTA& lf) :
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

	CallCreateSprite pD3DXCreateSprite;
	ID3DXSprite *pSprite = nullptr;

	int width = 0;
	int height = 0;
	int vfix = 0;
	int vbound = 0;
	int hbound = 0;
	int linespace = 0;
	int shadow = 0;
	
	PtrInfoList infos;
};

Draw * Draw::CreateDraw(u8& showing, void * hWnd, void * pD3DD, const char* fontName)
{
	DrawImpl* draw = new DrawImpl(showing, hWnd, pD3DD, fontName);
	return draw;
}

void Draw::DestoryDraw(Draw * draw)
{
	delete draw;
}

void DrawImpl::DrawInfos() {
	if (width == 0 || !pD3DD) return;

	if (pFont) {
		pD3DD->BeginScene();

		if (pSprite) {
			pSprite->Begin(SPRITE_BEGIN_FLAG);
		}
		RECT rect_shadow;

		for (const auto& info : infos) {
			rect_shadow = info->rect;
			if (info->format & DT_RIGHT) rect_shadow.right += shadow;
			else rect_shadow.left += shadow;
			if (info->format & DT_BOTTOM) rect_shadow.bottom += shadow;
			else rect_shadow.top += shadow;

			unsigned color_shadow = (0xFFFFFF & SHADOW_COLOR) | (0xFF << 24);

			pFont->DrawTextW(SPRITE info->text, -1, &rect_shadow, info->format, color_shadow);
		}

		for (const auto& info : infos) {
			pFont->DrawTextW(SPRITE info->text, -1, &info->rect, info->format, info->color);
		}

		if (pSprite) {
			pSprite->End();
		}
		pD3DD->EndScene();
	}
}

constexpr const unsigned DrawImpl::DftFormatList[];

void DrawImpl::AddInfoText(InfoType type, unsigned time, unsigned color, const char* text) {
	unsigned dead = time == ShowTimeInfinity ? TIME_MAX : Clock::Now() + time;
	constexpr int NumValidType = std::extent<decltype(DftFormatList)>::value;
	const unsigned format = (int)type < NumValidType ? DftFormatList[(int)type] : DftFormatList[(int)InfoType::All];

	LOG("Add text, type = %d", type);
	const int h = desca.Height < 0 ? -desca.Height : desca.Height;

	auto it = infos.end();

	bool creatNew = true;
	if (type != InfoType::Hello) {
		for (it = infos.begin(); it != infos.end(); ++it) {
			if ((*it)->type == type) {
				creatNew = false;
				break;
			}
		}
	}

	if (creatNew) {
		LOG("Create new Text.");
		infos.push_back(PtrInfo(new Info));
		it = --infos.end();
	}
	else {
		LOG("No need to create new Text.");
	}
	(*it)->type = type;
	(*it)->color = color;
	(*it)->deadTime = dead;
	(*it)->format = format;

	auto conRst = ConvertUtf8toUtf16((*it)->text, text);

	int text_width = (int)((conRst.cnt1 * 0.8 + conRst.cnt2 * 1.2 + conRst.cnt4 * 1.2) * h);
	LOG("Text is %s", text);
	LOG("Text width is %d", text_width);

	auto& rect = (*it)->rect;
	
	if (creatNew) {
		memset(&rect, 0, sizeof(rect));
		std::set<int> invalid_bottom, invalid_top;
		for (auto it2 = infos.begin(); it2 != infos.end(); ++it2) {
			if (it == it2 || (((*it)->format & DT_RIGHT) != ((*it2)->format & DT_RIGHT))) continue;
			invalid_top.insert((*it2)->rect.top);
			invalid_bottom.insert((*it2)->rect.bottom);
		}

		if ((*it)->format & DT_BOTTOM) {
			for (int bottom = height - vbound - vfix; ; bottom -= h + vbound) {
				if (invalid_bottom.find(bottom) == invalid_bottom.end()) {
					rect.bottom = bottom;
					break;
				}
			}
			rect.top = rect.bottom - h - linespace;
		}
		else {
			for (int top = vbound + vfix; ; top += h + linespace) {
				if (invalid_top.find(top) == invalid_top.end()) {
					rect.top = top;
					break;
				}
			}

			rect.bottom = rect.top + h + linespace;
		}
	} //if (creatNew)

	if ((*it)->format & DT_RIGHT) {
		rect.right = width - hbound;
		rect.left = rect.right - text_width;
	}
	else {
		rect.left = hbound;
		rect.right = rect.left + text_width;
	}

	LOG("top = %d, bottom = %d, left = %d, right = %d", rect.top, rect.bottom, rect.left, rect.right);

	showing = infos.size() > 0;
}

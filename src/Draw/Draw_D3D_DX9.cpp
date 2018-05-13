#include "Draw_D3D_DX9.h"

#include <Utils/ApiPack.h>
#include <Utils/Encoding.h>

#include <d3d9/d3dx9.h>

#define pd ((DX9_DATA*)dx9_data)

using CallCreateFont = decltype(D3DXCreateFontIndirectW)*;
using CallCreateSprite = decltype(D3DXCreateSprite)*;
constexpr char STR_D3DXCreateFontIndirect[] = "D3DXCreateFontIndirect";
constexpr char STR_D3DXCreateSprite[] = "D3DXCreateSprite";

struct DX9_DATA {
	IDirect3DDevice9* d3dd;
	D3DXFONT_DESCW desca;
	ID3DXFont *pFont;
	ID3DXSprite *pSprite;

	CallCreateFont pD3DXCreateFontIndirect;
};

bool Draw::D3D_DX9::BeginDraw(void* pD3DD)
{
	if (!pd) return false;
	pd->d3dd = (decltype(pd->d3dd))pD3DD;

	if (!pd->d3dd || D3D_OK != pd->d3dd->BeginScene()) {
		return false;
	}
	if (!pd->pD3DXCreateFontIndirect || D3D_OK != pd->pD3DXCreateFontIndirect(pd->d3dd, &pd->desca, &pd->pFont)) {
		pd->d3dd->EndScene();
		return false;
	}

	return true;
}

void Draw::D3D_DX9::EndDraw()
{
	if (!pd) return;

	if (pd->pFont) {
		pd->pFont->Release();
		pd->pFont = nullptr;
	}
	if(pd->d3dd) pd->d3dd->EndScene();
}

void Draw::D3D_DX9::DrawString(const WString& text, int count, const void * rect, unsigned format, unsigned color)
{
	if (pd->pFont) pd->pFont->DrawTextW(pd->pSprite, text.c_str(), count, (RECT*)rect, format, color);
}

Draw::D3D_DX9::~D3D_DX9()
{
	if (pd && pd->pFont) pd->pFont->Release();
	if (pd && pd->pSprite) pd->pSprite->Release();
	delete pd;

	this->dx9_data = nullptr;
}

Draw::D3D_DX9::D3D_DX9(const char * fontName, int fontSize)
{
	this->dx9_data = new DX9_DATA{ };

	auto wFontName = Encoding::Utf8ToUtf16(fontName);
	memcpy(pd->desca.FaceName, wFontName.c_str(), min(sizeof(pd->desca.FaceName), (wFontName.length() + 1) * 2));
	pd->desca.FaceName[sizeof(pd->desca.FaceName) - 1] = 0;

	pd->desca.Height = -fontSize;
	pd->desca.Weight = FW_NORMAL;
	pd->desca.CharSet = DEFAULT_CHARSET;
	pd->desca.OutputPrecision = OUT_OUTLINE_PRECIS;
	pd->desca.Quality = CLEARTYPE_QUALITY;

	pd->pD3DXCreateFontIndirect = (CallCreateFont)ApiPack::GetApi(STR_D3DXCreateFontIndirect);

	this->valid = pd->pD3DXCreateFontIndirect;
}

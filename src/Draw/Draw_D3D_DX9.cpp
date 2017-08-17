#include "Draw_D3D_DX9.h"

#include <Utils/ApiPack.h>
#include <Utils/EncodeHelper.h>

#include <d3d9/d3dx9.h>

#define pd ((DX9_DATA*)dx9_data)

using CallCreateFont = decltype(D3DXCreateFontIndirectW)*;
using CallCreateSprite = decltype(D3DXCreateSprite)*;
constexpr char STR_D3DXCreateFontIndirect[] = "D3DXCreateFontIndirect";
constexpr char STR_D3DXCreateSprite[] = "D3DXCreateSprite";

struct DX9_DATA {
	IDirect3DDevice9* const d3dd;
	D3DXFONT_DESCW desca;
	ID3DXFont *pFont;
	ID3DXSprite *pSprite;
};

void Draw::D3D_DX9::BeginScene()
{
	pd->d3dd->BeginScene();
	if (pd->pSprite) {
		pd->pSprite->Begin(D3DXSPRITE_ALPHABLEND);
	}
}

void Draw::D3D_DX9::EndScene()
{
	if (pd->pSprite) {
		pd->pSprite->End();
	}
	pd->d3dd->EndScene();
}

void Draw::D3D_DX9::DrawString(const WChar * text, int count, void * rect, unsigned format, unsigned color)
{
	pd->pFont->DrawTextW(pd->pSprite, text, count, (RECT*)rect, format, color);
}

Draw::D3D_DX9::~D3D_DX9()
{
	if (pd && pd->pFont) pd->pFont->Release();
	if (pd && pd->pSprite) pd->pSprite->Release();
	delete pd;
}

Draw::D3D_DX9::D3D_DX9(void * pD3DD, const char * fontName, int fontSize)
{
	this->dx9_data = new DX9_DATA{ (IDirect3DDevice9*)pD3DD,{}, nullptr, nullptr };

	ConvertUtf8toUtf16(pd->desca.FaceName, fontName);

	pd->desca.Height = -fontSize;
	pd->desca.Weight = FW_NORMAL;
	pd->desca.CharSet = DEFAULT_CHARSET;
	pd->desca.OutputPrecision = OUT_OUTLINE_PRECIS;
	pd->desca.Quality = CLEARTYPE_QUALITY;

	CallCreateFont pD3DXCreateFontIndirect = (CallCreateFont)ApiPack::GetApi(STR_D3DXCreateFontIndirect);
	if(pD3DXCreateFontIndirect) pD3DXCreateFontIndirect(pd->d3dd, &pd->desca, &pd->pFont);
	if (!pd->pFont) {
		delete pd;
		this->dx9_data = nullptr;
		this->valid = false;
		return;
	}

	CallCreateSprite pD3DXCreateSprite = (CallCreateSprite)ApiPack::GetApi(STR_D3DXCreateSprite);;
	if(pD3DXCreateSprite) pD3DXCreateSprite(pd->d3dd, &pd->pSprite);
	this->valid = true;
}

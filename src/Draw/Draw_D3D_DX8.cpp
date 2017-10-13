#include "Draw_D3D_DX8.h"

#include <Utils/EncodeHelper.h>

#include <d3d8/d3dx8.h>

#define pd ((DX8_DATA*)dx8_data)

using CallCreateFont = decltype(D3DXCreateFontIndirect)*;

struct DX8_DATA {
	IDirect3DDevice8* d3dd;
	LOGFONTA lf;
	ID3DXFont *pFont;

	CallCreateFont pD3DXCreateFontIndirect;
};

bool Draw::D3D_DX8::BeginDraw(void* pD3DD)
{
	if (!pd) return false;
	pd->d3dd = (decltype(pd->d3dd))pD3DD;

	if (!pd->d3dd || D3D_OK != pd->d3dd->BeginScene()) {
		return false;
	}
	if (!pd->pD3DXCreateFontIndirect || D3D_OK != pd->pD3DXCreateFontIndirect(pd->d3dd, &pd->lf, &pd->pFont)) {
		pd->d3dd->EndScene();
		return false;
	}

	return true;
}

void Draw::D3D_DX8::EndDraw()
{
	if (!pd) return;

	if (pd->pFont) {
		pd->pFont->Release();
		pd->pFont = nullptr;
	}
	if(pd->d3dd) pd->d3dd->EndScene();
}

void Draw::D3D_DX8::DrawString(const WChar * text, int count, void * rect, unsigned format, unsigned color)
{
	if (pd->pFont) pd->pFont->DrawTextW(text, count, (RECT*)rect, format, color);
}

Draw::D3D_DX8::~D3D_DX8()
{
	if(pd && pd->pFont) pd->pFont->Release();
	delete pd;

	this->dx8_data = nullptr;
}

Draw::D3D_DX8::D3D_DX8(const char * fontName, int fontSize)
{
	this->dx8_data = new DX8_DATA{ };

	WChar buff[sizeof(pd->lf.lfFaceName)];
	ConvertUtf8toUtf16(buff, fontName);
	WideCharToMultiByte(CP_OEMCP, 0, buff, -1, pd->lf.lfFaceName, sizeof(pd->lf.lfFaceName), 0, 0);

	pd->lf.lfHeight = -fontSize;
	pd->lf.lfWeight = FW_NORMAL;
	pd->lf.lfCharSet = DEFAULT_CHARSET;
	pd->lf.lfOutPrecision = OUT_OUTLINE_PRECIS;
	pd->lf.lfQuality = CLEARTYPE_QUALITY;

	pd->pD3DXCreateFontIndirect = D3DXCreateFontIndirect;

	this->valid = true;
}

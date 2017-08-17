#include "Draw_D3D_DX8.h"

#include <Utils/EncodeHelper.h>

#include <d3d8/d3dx8.h>

#define pd ((DX8_DATA*)dx8_data)

struct DX8_DATA {
	IDirect3DDevice8* const d3dd;
	LOGFONTA lf;
	ID3DXFont *pFont;
};

void Draw::D3D_DX8::BeginScene()
{
	pd->d3dd->BeginScene();
}

void Draw::D3D_DX8::EndScene()
{
	pd->d3dd->EndScene();
}

void Draw::D3D_DX8::DrawString(const WChar * text, int count, void * rect, unsigned format, unsigned color)
{
	pd->pFont->DrawTextW(text, count, (RECT*)rect, format, color);
}

Draw::D3D_DX8::~D3D_DX8()
{
	if(pd && pd->pFont) pd->pFont->Release();
	delete pd;
}

Draw::D3D_DX8::D3D_DX8(void * pD3DD, const char * fontName, int fontSize)
{
	this->dx8_data = new DX8_DATA{ (IDirect3DDevice8*)pD3DD,{ }, nullptr };

	WChar buff[sizeof(pd->lf.lfFaceName)];
	ConvertUtf8toUtf16(buff, fontName);
	WideCharToMultiByte(CP_OEMCP, 0, buff, -1, pd->lf.lfFaceName, sizeof(pd->lf.lfFaceName), 0, 0);

	pd->lf.lfHeight = -fontSize;
	pd->lf.lfWeight = FW_NORMAL;
	pd->lf.lfCharSet = DEFAULT_CHARSET;
	pd->lf.lfOutPrecision = OUT_OUTLINE_PRECIS;
	pd->lf.lfQuality = CLEARTYPE_QUALITY;

	D3DXCreateFontIndirect(pd->d3dd, &pd->lf, &pd->pFont);

	if (!pd->pFont) {
		delete pd;
		this->dx8_data = nullptr;
		this->valid = false;
	}
	else {
		this->valid = true;
	}
}

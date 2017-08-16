#include "Draw_D3D.h"
#include "Draw_D3D_DX8.h"
#include "Draw_D3D_DX9.h"

using namespace Draw;

D3D * Draw::D3D::GetD3D(int dx9, void* pD3DD, const char* fontName, unsigned fontSize)
{
	D3D * res;
	if (dx9) {
		res = new D3D_DX9(pD3DD, fontName, fontSize);
	}
	else {
		res = new D3D_DX8(pD3DD, fontName, fontSize);
	}
	if (!res->Valid()) {
		delete res;
		res = nullptr;
	}
	return res;
}

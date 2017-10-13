#include "Draw_D3D.h"
#include "Draw_D3D_DX8.h"
#include "Draw_D3D_DX9.h"

using namespace Draw;

D3D * Draw::D3D::GetD3D(int dx9, const char* fontName, int fontSize)
{
	D3D * res;
	if (dx9) {
		res = new D3D_DX9(fontName, fontSize);
	}
	else {
		res = new D3D_DX8(fontName, fontSize);
	}
	if (!res->Valid()) {
		delete res;
		res = nullptr;
	}
	return res;
}

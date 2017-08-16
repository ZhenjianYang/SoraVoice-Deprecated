#pragma once

#include "Draw_D3D.h"

namespace Draw {

	class D3D_DX9 : public D3D
	{
	public:
		virtual void BeginScene();
		virtual void EndScene();

		virtual void DrawString(const WChar* text, int count, void* rect, unsigned format, unsigned color);

		virtual ~D3D_DX9();

		D3D_DX9(void* pD3DD, const char* fontName, unsigned fontSize);

	private:
		void* dx9_data;
	};
}
#pragma once

#include <string>

namespace Draw {
	using WString = std::wstring;

	class D3D
	{
	public:
		virtual bool BeginDraw(void* pD3DD) = 0;
		virtual void EndDraw() = 0;

		virtual void DrawString(const WString& text, int count, const void* rect, unsigned format, unsigned color) = 0;

		virtual ~D3D() { }

		bool Valid() const { return this->valid; }

		static D3D * GetD3D(int dx9, const char* fontName, int fontSize);

	protected:
		bool valid;
	};
}
#pragma once

namespace Draw {
	using WChar = wchar_t;
	class D3D
	{
	public:
		virtual void BeginScene() = 0;
		virtual void EndScene() = 0;

		virtual void DrawString(const WChar* text, int count, void* rect, unsigned format, unsigned color) = 0;

		virtual ~D3D() { }

		bool Valid() const { return this->valid; }

		static D3D * GetD3D(int dx9, void* pD3DD, const char* fontName, int fontSize);

	protected:
		bool valid;
	};
}
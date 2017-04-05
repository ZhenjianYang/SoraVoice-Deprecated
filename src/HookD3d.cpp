
#define CINTERFACE 1
#include <d3d8/d3dx8.h>

#include "HookD3d.h"
#include "SoraVoice.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	HRESULT WINAPI Hooked_IDirect3DDevice8_Present(IDirect3DDevice8 * D3DD, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);

#ifdef __cplusplus
}
#endif // __cplusplus

static SoraVoice* sv = nullptr;
static void* pPresent = nullptr;

HRESULT WINAPI Hooked_IDirect3DDevice8_Present(IDirect3DDevice8 * D3DD, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion) {
	if (sv) {
		sv->Show();
	}
	using Call = decltype(D3DD->lpVtbl->Present);
	return ((Call)pPresent)(D3DD, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

void* Hook_IDirect3DDevice8_Present(IDirect3DDevice8 * D3DD, void * sv)
{
	if (!D3DD) return nullptr;
	DWORD tmp;
	if (!VirtualProtect(&D3DD->lpVtbl->Present, 4, PAGE_EXECUTE_READWRITE, &tmp)) return nullptr;

	using Call = decltype(D3DD->lpVtbl->Present);
	if (pPresent) D3DD->lpVtbl->Present = (Call)pPresent;
	
	pPresent = D3DD->lpVtbl->Present;
	D3DD->lpVtbl->Present = Hooked_IDirect3DDevice8_Present;
	::sv = (SoraVoice*)sv;

	return pPresent;
}


#define CINTERFACE 1

#ifdef ZA
#include <d3d9.h>
#define D3D IDirect3D9
#define D3DD IDirect3DDevice9
#else
#include <d3d8/d3dx8.h>
#define D3D IDirect3D8
#define D3DD IDirect3DDevice8
#endif

#include <dinput.h>
#define DID IDirectInputDevice8A

#include "Hook.h"
#include "SoraVoice.h"
#include "InitParam.h"

HRESULT WINAPI Hooked_Present(D3DD* pD3DD, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);
HRESULT WINAPI Hooked_GetDeviceState(DID* This, DWORD cbData, LPVOID lpvData);

static InitParam* ip = nullptr;

using CallPresent = decltype(Hooked_Present);
static CallPresent* pPresent = nullptr;

using CallGetDeviceState = decltype(Hooked_GetDeviceState);
CallGetDeviceState* pGetDeviceState = nullptr;

HRESULT WINAPI Hooked_Present(D3DD* pD3DD, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion) {
	if (ip && ip->sv) {
		CallPresent* bak = pD3DD->lpVtbl->Present;
		((SoraVoice*)ip->sv)->Show();
		pD3DD->lpVtbl->Present = bak;
	}
	return pPresent(pD3DD, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

HRESULT WINAPI Hooked_GetDeviceState(DID * This, DWORD cbData, LPVOID lpvData)
{
	auto rst = pGetDeviceState(This, cbData, lpvData);

	if (ip && cbData == 0x100) {
		ip->addrs.p_keys = (const char*)lpvData;
			
		if (ip->sv) {
			((SoraVoice*)ip->sv)->Input();
		}
	}
	return rst;
}

void* Hook_D3D_Present(void* pD3DD)
{
	if (!pD3DD) return nullptr;

	if (!pPresent) {
		DWORD tmp;
		D3DD *p = (D3DD*)pD3DD;
		if (!VirtualProtect(&p->lpVtbl->Present, 4, PAGE_EXECUTE_READWRITE, &tmp)) return nullptr;

		pPresent = p->lpVtbl->Present;
		p->lpVtbl->Present = Hooked_Present;
	}

	return (void*)pPresent;
}

void * Hook_DI_GetDeviceState(void * pDID)
{
	if(!pDID) return nullptr;

	if (!pGetDeviceState) {
		DWORD tmp;
		DID* p = (DID*)pDID;
		if (!VirtualProtect(&p->lpVtbl->GetDeviceState, 4, PAGE_EXECUTE_READWRITE, &tmp)) return nullptr;

		pGetDeviceState = p->lpVtbl->GetDeviceState;
		p->lpVtbl->GetDeviceState = Hooked_GetDeviceState;
	}
	return (void*)pGetDeviceState;
}

void InitHook_SetInitParam(void * ip)
{
	::ip = (InitParam*)ip;
}

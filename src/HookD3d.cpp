
#define CINTERFACE 1
#include <d3d8/d3dx8.h>

#include "HookD3d.h"
#include "SoraVoice.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	HRESULT WINAPI HK_IDirect3D8_CreateDevice(IDirect3D8 * D3D, UINT Adapter, D3DDEVTYPE DeviceType,
		HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS * pPresentationParameters,
		IDirect3DDevice8 ** ppReturnedDeviceInterface);

	HRESULT WINAPI HK_IDirect3DDevice8_Present(IDirect3DDevice8 * D3DD, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);

#ifdef __cplusplus
}
#endif // __cplusplus

static InitParam* ip;

static void* pCreateDevice;
static void* pPresent;

IDirect3D8* WINAPI HK_Direct3D_Create8(unsigned SDKVersion)
{
	IDirect3D8* rst = Direct3DCreate8((UINT)SDKVersion);
	if (rst) {
		pCreateDevice = rst->lpVtbl->CreateDevice;
		DWORD tmp;
		VirtualProtect(&rst->lpVtbl->CreateDevice, 4, PAGE_EXECUTE_READWRITE, &tmp);
		rst->lpVtbl->CreateDevice = HK_IDirect3D8_CreateDevice;
	}
	return rst;
}

HRESULT WINAPI HK_IDirect3D8_CreateDevice(IDirect3D8 * D3D, UINT Adapter, D3DDEVTYPE DeviceType, 
	HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS * pPresentationParameters, 
	IDirect3DDevice8 ** ppReturnedDeviceInterface)
{
	using Call = decltype(D3D->lpVtbl->CreateDevice);
	HRESULT rst = ((Call)pCreateDevice)(D3D,Adapter,DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
	if (rst == D3D_OK) {
		IDirect3DDevice8* D3DD = *ppReturnedDeviceInterface;
		pPresent = D3DD->lpVtbl->Present;
		DWORD tmp;
		VirtualProtect(&D3DD->lpVtbl->Present, 4, PAGE_EXECUTE_READWRITE, &tmp);
		D3DD->lpVtbl->Present = HK_IDirect3DDevice8_Present;
	}
	return rst;
}

HRESULT WINAPI HK_IDirect3DDevice8_Present(IDirect3DDevice8 * D3DD, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion) {
	if (ip && ip->sv) {
		((SoraVoice*)ip->sv)->Show(D3DD);
	}
	using Call = decltype(D3DD->lpVtbl->Present);
	return ((Call)pPresent)(D3DD, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

void InitHook(void * p)
{
	ip = (InitParam*)p;
}


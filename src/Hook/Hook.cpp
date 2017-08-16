#include "Hook.h"

#include <Windows.h>

#ifndef WINAPI
#define WINAPI _stdcall
#endif // !WINAPI


long WINAPI Hooked_Present(void* pD3DD, void* pSourceRect, void* pDestRect, void* hDestWindowOverride, void* pDirtyRegion);
long WINAPI Hooked_GetDeviceState(void* This, unsigned cbData, void* lpvData);

using CallPresent = decltype(Hooked_Present)*;
static CallPresent pPresent = nullptr;
static Hook::HookCallBack cbPresent;

using CallGetDeviceState = decltype(Hooked_GetDeviceState)*;
CallGetDeviceState pGetDeviceState = nullptr;
static Hook::HookCallBack cbGetDeviceState;
static void **pp_keys;

inline void** GetAddrGetDeviceState(void* pDID);
inline void** GetAddrPresent(void* pD3DD);
inline void** GetAddrPresentDX8(void* pD3DD);

long WINAPI Hooked_Present(void* pD3DD, void* pSourceRect, void* pDestRect, void* hDestWindowOverride, void* pDirtyRegion) {
	if (cbPresent) {
		cbPresent();
	}
	return pPresent(pD3DD, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

long WINAPI Hooked_GetDeviceState(void* This, unsigned cbData, void* lpvData)
{
	auto rst = pGetDeviceState(This, cbData, lpvData);

	if (cbGetDeviceState && cbData == 0x100) {
		if(pp_keys) *pp_keys = lpvData;
			
		cbGetDeviceState();
	}
	return rst;
}

void* Hook::Hook_D3D_Present(void* pD3DD, int dx9, HookCallBack callback)
{
	if (!pD3DD) return nullptr;

	if (!pPresent) {
		DWORD oldProtect, oldProtect2;
		void** addrPresent = dx9 ? GetAddrPresent(pD3DD) : GetAddrPresentDX8(pD3DD);
		if (!VirtualProtect(addrPresent, 4, PAGE_EXECUTE_READWRITE, &oldProtect)) return nullptr;

		pPresent = (CallPresent)*addrPresent;
		*addrPresent = Hooked_Present;

		VirtualProtect(addrPresent, 4, oldProtect, &oldProtect2);
	}

	return (void*)pPresent;
}

void* Hook::Hook_DI_GetDeviceState(void* pDID, HookCallBack callback, void** pp_keys)
{
	if(!pDID) return nullptr;

	if (!pGetDeviceState) {
		DWORD oldProtect, oldProtect2;
		void** addrGetDeviceState = GetAddrGetDeviceState(pDID);
		if (!VirtualProtect(addrGetDeviceState, 4, PAGE_EXECUTE_READWRITE, &oldProtect)) return nullptr;

		pGetDeviceState = (CallGetDeviceState)*addrGetDeviceState;
		*addrGetDeviceState = Hooked_GetDeviceState;

		VirtualProtect(addrGetDeviceState, 4, oldProtect, &oldProtect2);
	}
	return (void*)pGetDeviceState;
}

#pragma once

#ifndef D3DCALL
#define D3DCALL __stdcall
#endif // !D3DCALL

struct IDirect3D8;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	IDirect3D8* D3DCALL HK_Direct3D_Create8(unsigned SDKVersion);

#ifdef __cplusplus
}
#endif // __cplusplus

void InitHook(void* sv);


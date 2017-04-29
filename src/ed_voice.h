#pragma once

#define SVCALL __stdcall

#ifndef D3DCALL
#define D3DCALL __stdcall
#endif // !D3DCALL

#ifdef _SVDLL
#define SVDECL
#else
#define SVDECL __declspec(dllimport)
#endif // _SVDLL

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	SVDECL void SVCALL Init(void*);
	SVDECL void SVCALL End(void*);

	SVDECL void SVCALL Play(void*, void*);
	SVDECL void SVCALL Stop(void*);
#ifdef __cplusplus
}
#endif // __cplusplus


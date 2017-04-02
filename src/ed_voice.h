#pragma once

#define SVCALL __stdcall
#define D3DCALL __stdcall

#ifdef _SVDLL
#define SVDECL _declspec(dllexport)
#else
#define SVDECL _declspec(dllimport)
#endif // _SVDLL

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	SVDECL void SVCALL Init(void*);
	SVDECL void SVCALL End(void*);

	SVDECL void SVCALL Play(void*, void*);
	SVDECL void SVCALL Stop(void*);

	SVDECL void SVCALL Input(void*);

	SVDECL void* D3DCALL Direct3DCreate8(unsigned, void*);
#ifdef __cplusplus
}
#endif // __cplusplus


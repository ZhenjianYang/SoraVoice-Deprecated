#pragma once

#ifndef SVCALL
#define SVCALL __stdcall
#endif // !SVCALL

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	long SVCALL Hooked_DirectInput8Create(void* hinst, unsigned dwVersion, void* riidltf, void **ppvOut, void *punkOuter);

#ifdef __cplusplus
}
#endif // __cplusplus



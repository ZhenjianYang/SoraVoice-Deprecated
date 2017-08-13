#pragma once

#ifndef SVCALL
#define SVCALL __stdcall
#endif // !SVCALL

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	long SVCALL Hooked_DirectSoundCreate(void* pcGuidDevice, void **ppDS, void* pUnkOuter);

#ifdef __cplusplus
}
#endif // __cplusplus



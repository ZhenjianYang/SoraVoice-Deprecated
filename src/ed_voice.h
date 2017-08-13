#pragma once

#define SVCALL __stdcall

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	void SVCALL Init(void*);
	void SVCALL End(void*);

	void SVCALL Play(void*, void*);
	void SVCALL Stop(void*);

#ifndef ZA
	unsigned SVCALL LoadDat(const char*, void*);
#endif // !ZA
#ifdef __cplusplus
}
#endif // __cplusplus


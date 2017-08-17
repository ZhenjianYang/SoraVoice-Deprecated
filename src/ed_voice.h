#pragma once

#ifndef SVCALL
#define SVCALL __stdcall
#endif

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	void SVCALL Start(void*);
	void SVCALL End(void*);

#ifdef __cplusplus
}
#endif // __cplusplus


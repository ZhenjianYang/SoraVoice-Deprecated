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
	int SVCALL LoadScn(const char* name, char* buff);
	int SVCALL LoadScns(void* p_PScns, int id, char **pp_t);
#endif // !ZA
#ifdef __cplusplus
}
#endif // __cplusplus


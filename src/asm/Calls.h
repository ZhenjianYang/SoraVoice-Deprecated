#pragma once

#ifndef SVCALL
#define SVCALL __stdcall
#endif

#ifdef __cplusplus
extern "C" {
#endif

void SVCALL ASM_Play(const char* v);
void SVCALL ASM_Stop();
int SVCALL ASM_Init();

int SVCALL ASM_LoadScn(const char* name, char* buff);
int SVCALL ASM_LoadScns(void* p_PScns, int id, char **pp_t);

void SVCALL ASM_RdScnPath(char* path);

#ifdef __cplusplus
}
#endif

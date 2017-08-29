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

int SVCALL ASM_LoadScn(char* buff, int idx, int game);
int SVCALL ASM_LoadScns(char* buffs[], int idx_main, int game);

void SVCALL ASM_RdScnPath(char* path);

#ifdef __cplusplus
}
#endif

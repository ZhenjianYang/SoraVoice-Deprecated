#pragma once

#ifndef SVCALL
#define SVCALL __stdcall
#endif

#ifndef CCALL
#define CCALL __cdecl
#endif

#ifdef __cplusplus
extern "C" {
#endif

void SVCALL ASM_Play(const char* v);
void SVCALL ASM_Stop();
int SVCALL ASM_Init();

int SVCALL ASM_LoadScn(char* buff, int idx, int game);
int SVCALL ASM_LoadScns(char* buffs[], int idx_main, int game);

int CCALL ASM_RdScnPath(void* ret, char* buff, const char* format, char* dir, const char* scn);

void SVCALL ASM_Show(void* pD3DD);

int CCALL ASM_LoadDat(void*, void* buff, int idx, unsigned offset, unsigned size);
int SVCALL ASM_DecompressDat(void** compressed, void** uncompressed);

void* SVCALL ASM_DrawTexts(const char * text, void * buffer, unsigned stride, unsigned color_index);

#ifdef __cplusplus
}
#endif

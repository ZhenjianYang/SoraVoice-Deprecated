#include "Calls.h"
#include <SoraVoice.h>
#include <DrawTexts/DrawTexts.h>

void SVCALL ASM_Play(const char* v) { SoraVoice::Play(v); }
void SVCALL ASM_Stop() { SoraVoice::Stop(); };
int SVCALL ASM_Init() { return SoraVoice::Init(); };

void SVCALL ASM_Show(void* pD3DD) { SoraVoice::Show(pD3DD); };
void* SVCALL ASM_DrawTexts(const char * text, void * buffer, unsigned stride, unsigned color_index) { return DrawTexts::DrawTexts(text, buffer, stride, color_index); }
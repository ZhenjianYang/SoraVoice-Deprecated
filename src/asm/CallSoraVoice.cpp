#include "asm_callee.h"
#include <SoraVoice.h>

void SVCALL ASM::Play(const char* v) { SoraVoice::Play(v); }
void SVCALL ASM::Stop() { SoraVoice::Stop(); };
bool SVCALL ASM::Init() { return SoraVoice::Init(); };

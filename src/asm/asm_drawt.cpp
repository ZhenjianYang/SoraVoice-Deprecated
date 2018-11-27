#include "asm.h"
#include "inline_asm.h"
#include "Calls.h"
#include <SVData.h>

NACKED void ASM::drawt() {
	INLINE_ASM{
		jmp ASM_DrawTexts;
	}
}

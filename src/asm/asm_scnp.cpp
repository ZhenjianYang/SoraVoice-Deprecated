#include "asm.h"
#include "inline_asm.h"
#include "Calls.h"
#include <SVData.h>

NACKED void ASM::scnp() {
	INLINE_ASM {
		call    ASM_RdScnPath;
		jmp     dword ptr[SV.jcs.scnp.to];
	}
}

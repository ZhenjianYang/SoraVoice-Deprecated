#include "asm.h"
#include "inline_asm.h"
#include "Calls.h"
#include <SVData.h>

NACKED void ASM::ldat() {
	INLINE_ASM{
		call    ASM_LoadDat;
		test    eax, eax;
		je      ldat_ori;
		ret;

	ldat_ori:
		jmp    dword ptr[SV.jcs.ldat.next];
	}
}
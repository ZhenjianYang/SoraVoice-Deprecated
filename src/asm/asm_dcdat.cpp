#include "asm.h"
#include "inline_asm.h"
#include "Calls.h"
#include <SVData.h>

NACKED void ASM::dcdat() {
	INLINE_ASM{
		push    ebx;
		push    edi;
		call    ASM_DecompressDat;
		test    eax, eax;
		je      dcdat_ori;
		ret;

	dcdat_ori:
		jmp      dword ptr[SV.jcs.dcdat.next];
	}
}

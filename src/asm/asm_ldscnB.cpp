#include "asm.h"
#include "inline_asm.h"
#include "Calls.h"
#include <SVData.h>

NACKED void ASM::ldscnB() {
	INLINE_ASM{
		mov     edx, dword ptr[esp + 8];
		mov     eax, edx;
		shr     eax, 16;

		cmp     eax, 1;
		je      ldscnB_call;
		cmp     eax, 0x21;
		jne     ldscnB_call_ori;
	ldscnB_call:
		mov     ecx, dword ptr[esp + 4];
		movzx   edx, dx
		push    dword ptr[SV.game];
		push    edx;
		push    ecx;
		call    ASM_LoadScn;
		test    eax, eax;
		jz      ldscnB_call_ori;

	//ldscnB_return:
		or      al, 1
		ret;

	ldscnB_call_ori:
		jmp     dword ptr[SV.jcs.ldscnB.to];
	}
}

#include "asm.h"
#include "inline_asm.h"
#include "Calls.h"
#include <SVData.h>

NACKED void ASM::ldquizB() {
	INLINE_ASM{
		push    ecx
		shr     ecx, 16;
		cmp     ecx, 0x22;
		pop     ecx;
		jne     ldquizB_call_ori;

		push    eax;
		push    ecx;
		push    edx;

		push    4;
		and     ecx, 0xFFFF
		push    ecx;
		push    dword ptr[ebx];
		call    ASM_LoadScn;
		test    eax, eax;

		pop     edx;
		pop     ecx;
		jne     short ldquizB_ret;

		pop     eax;
	ldquizB_call_ori:
		push    ecx;
		mov     edx, dword ptr[SV.addrs.addr_quizp];
		mov     edx, dword ptr[edx];
		jmp     dword ptr[SV.jcs.ldquizB.to];

	ldquizB_ret:
		add     dword ptr[ebx], eax;
		pop     eax;
		mov     eax, 1;
		ret
	}
}


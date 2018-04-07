#include "asm.h"
#include "inline_asm.h"
#include "Calls.h"
#include <SVData.h>

NACKED void ASM::ldquiz() {
	INLINE_ASM{
		mov     eax, dword ptr[esp + 8];
		shr     eax, 16;
		cmp     eax, 0x22
		jne     ldquiz_call_ori
		
		mov     eax, dword ptr[esp + 8];
		push    ecx
		push    edx

		push    4;
		and     eax, 0xFFFF
		push    eax
		mov     eax, dword ptr[esp + 4 + 0x10];
		push    dword ptr[eax];
		call    ASM_LoadScn;
		test    eax, eax;

		pop     edx
		pop     ecx
		jne     short ldquiz_ret;

	ldquiz_call_ori:
		push    ecx
		mov     eax, dword ptr[SV.addrs.addr_quizp]
		mov     eax, dword ptr[eax]
		jmp     dword ptr[SV.jcs.ldquiz.to];

	ldquiz_ret:
		mov     ecx, dword ptr[esp + 4];
		add     dword ptr[ecx], eax;
		mov     eax, 1;
		ret
	}
}


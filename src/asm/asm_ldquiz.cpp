#include "asm.h"
#include "inline_asm.h"
#include "Calls.h"
#include <SVData.h>

NACKED void ASM::ldquiz() {
	INLINE_ASM{
		mov     ecx, edx;
		shr     ecx, 16;
		cmp     ecx, 0x22
		jne     ldquiz_call_ori

		push    eax;
		push    4;
		and     edx, 0xFFFF
		push    edx;
		push    dword ptr[eax];
		call    ASM_LoadScn;
		test    eax, eax;
		pop     ecx;

		jne     short ldquiz_ret;

	ldquiz_call_ori:
		jmp     dword ptr[SV.jcs.ldquiz.to];

	ldquiz_ret:
		add     dword ptr[ecx], eax;
		mov     eax, 1;
		ret
	}
}


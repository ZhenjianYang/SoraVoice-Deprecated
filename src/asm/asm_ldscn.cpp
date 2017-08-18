#include "asm.h"
#include "inline_asm.h"
#include "Calls.h"
#include <SVData.h>

NACKED void ASM::ldscn() {
	INLINE_ASM{
		sub     esp, 0x10;
		push    ebx;
		push    ebp;
		push    esi;
		mov     esi,[esp + 0x20];
		push    edi;

		push    eax;
		push    ecx;
		push    edx;

		push    dword ptr[SV.addrs.addr_ppscn];
		push    eax;
		push    esi;
		call    ASM_LoadScns;
		test    eax, eax;

		pop     edx;
		pop     ecx;
		pop     eax;

		jne     ldscn_iscn;

	//ldscn_call_ori:
		jmp     dword ptr[SV.jcs.ldscn.next];

	ldscn_iscn:
		jmp     dword ptr[SV.addrs.addr_iscn];
	}
}


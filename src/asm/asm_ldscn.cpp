#include "asm.h"
#include "inline_asm.h"
#include "Calls.h"
#include <SVData.h>

NACKED void ASM::ldscn() {
	INLINE_ASM{
		push    dword ptr[SV.addrs.addr_ppscn];
		push    eax;
		push    ebx;
		call    ASM_LoadScns;
		test    eax, eax;

		jne     ldscn_iscn;

	//ldscn_call_ori:
		mov     ebp, 8
		jmp     dword ptr[SV.jcs.ldscn.next];

	ldscn_iscn:
		mov     ecx, dword ptr[ebx];
		mov     dword ptr[ebx - 0x18], ecx;
		lea     eax, [esi + 0x2C];
		sub     eax, ebx;
		jnz     ldscn_to;
		mov     dword ptr[ebx - 0x1C], ecx;

	ldscn_to:
		jmp     dword ptr[SV.addrs.addr_iscn];
	}
}


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
		cmp     dword ptr[SV.game], SORA_FC;
		jne     ldscn_to_tits23;
		
		mov     dword ptr[ebx - 0x1C], ecx;
		jmp     dword ptr[SV.addrs.addr_iscn];
		
	ldscn_to_tits23:
		mov     dword ptr[ebx - 0x18], ecx;
		jmp     dword ptr[SV.addrs.addr_iscn];
	}
}


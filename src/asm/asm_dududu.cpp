#include "asm_callee.h"
#include <SVData.h>

void dududu() {
	_asm {
		cmp     dword ptr [sv.sora], 0
		jne     dududu_sora
		
		cmp     dword ptr [sv.tits], 0
		jne     dududu_tits
		
	/////zero
		cmp     dword ptr [sv.order.disableDududu], 0
		je      dududu_zero_to
		mov     dword ptr [esp + 0x10], 0
	dududu_zero_to:
		jmp     dword ptr [sv.jcs.dududu.to]
	
	/////sora
	dududu_sora:
		cmp     dword ptr [sv.order.disableDududu], 0
		je      short dududu_sora_over

		mov     eax, dword ptr [sv.addrs.addr_mute]
		cmp     dword ptr [eax], 0
		jne     short dududu_sora_over

		mov     dword ptr [eax], 1
		add     esp, 4
		call    dword ptr [sv.jcs.dududu.to]
		mov     eax, dword ptr [sv.addrs.addr_mute]
		mov     dword ptr [eax], 0
		jmp     dword ptr [sv.jcs.dududu.next]
	dududu_sora_over:
		jmp     dword ptr [sv.jcs.dududu.to]
	
	/////tits
	dududu_tits:
		cmp     dword ptr [sv.order.disableDududu], 0
		jne     short dududu_tits_to
		
		mov     eax, dword ptr [sv.addrs.addr_mute]
		cmp     dword ptr [eax], 0
		jne     short dududu_tits_to
		
		ret

	dududu_tits_to:
		push    dword ptr [sv.jcs.dududu.to]
		ret     4
	}
}

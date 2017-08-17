#include "asm_callee.h"
#include <SVData.h>

void dududu() {
	_asm {
		cmp     dword [sv.sora], 0
		jne     dududu_sora
		
		cmp     dword [sv.tits], 0
		jne     dududu_tits
		
	/////zero
		cmp     dword [sv.order.disableDududu], 0
		je      dududu_zero_to
		mov     dword [esp + 0x10], 0
	dududu_zero_to:
		jmp     dword [sv.jcs.dududu.to]
	
	/////sora
	dududu_sora:
		cmp     dword [sv.order.disableDududu], 0
		je      short dududu_sora_over

		mov     eax, dword [sv.addrs.addr_mute]
		cmp     dword [eax], 0
		jne     short close_dududu_over

		mov     dword [eax], 1
		add     esp, 4
		call    dword [sv.jcs.dududu.to]
		mov     eax, dword [sv.addrs.addr_mute]
		mov     dword [eax], 0
		jmp     dword [sv.jcs.dududu.next]
	dududu_sora_over:
		jmp     dword [sv.jcs.dududu.to]
	
	/////tits
	dududu_tits:
		cmp     dword [sv.order.disableDududu], 0
		jne     short dududu_tits_to
		
		mov     eax, dword [sv.addrs.addr_mute]
		cmp     dword [eax], 0
		jne     short dududu_tits_to
		
		ret

	dududu_tits_to:
		push    dword [sv.jcs.dududu.to]
		ret     4
	}
}

#include "asm_callee.h"
#include <SVData.h>

void dlgse() {
	_asm {
		push    dword ptr [sv.order.disableDialogSE]

		push    ecx
		push    edx
		call    dword ptr [ASM::Stop]
		pop     edx
		pop     ecx

		pop     eax

		cmp     dword ptr [sv.sora], 0
		jne     dlgse_sora
		cmp     dword ptr [sv.tits], 0
		jne     dlgse_tits
		
	/////zero
		test    eax, eax
		je      dlgse_zero_to
		mov     dword ptr [esp + 0x10], 0
	dlgse_zero_to:
		jmp     dword ptr [sv.jcs.dlgse.to]
	
	/////sora
	dlgse_sora:
		test    eax, eax
		je      short dlgse_sora_over

		mov     eax, dword ptr [sv.addrs.addr_mute]
		cmp     dword ptr [eax], 0
		jne     short dlgse_sora_over

		mov     dword ptr [eax], 1
		add     esp, 4
		call    dword ptr [sv.jcs.dlgse.to]
		mov     eax, dword ptr [sv.addrs.addr_mute]
		mov     dword ptr [eax], 0
		jmp     dword ptr [sv.jcs.dlgse.next]
	dlgse_sora_over:
		jmp     dword ptr [sv.jcs.dlgse.to]
	
	/////tits
	dlgse_tits:
		test    eax, eax
		jne     dlgse_tits_to
		
		mov     eax, dword ptr [sv.addrs.addr_mute]
		cmp     dword ptr [eax], 0
		jne     short dlgse_tits_to
		
		ret

	dlgse_tits_to:
		push    dword ptr [sv.jcs.dlgse.to]
		ret     4
	}
}

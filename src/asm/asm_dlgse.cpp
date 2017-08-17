#include "asm_callee.h"
#include <SVData.h>

void dlgse() {
	_asm {
		push    dword [sv.order.disableDialogSE]

		push    ecx
		push    edx
		call    dword [ASM::Stop]
		pop     edx
		pop     ecx

		pop     eax

		cmp     dword [sv.sora], 0
		jne     dlgse_sora
		cmp     dword [sv.tits], 0
		jne     dlgse_tits
		
	/////zero
		test    eax, eax
		je      dlgse_zero_to
		mov     dword [esp + 0x10], 0
	dlgse_zero_to:
		jmp     dword [sv.jcs.dlgse.to]
	
	/////sora
	dlgse_sora:
		test    eax, eax
		je      short dlgse_sora_over

		mov     eax, dword [sv.addrs.addr_mute]
		cmp     dword [eax], 0
		jne     short close_dlgse_over

		mov     dword [eax], 1
		add     esp, 4
		call    dword [sv.jcs.dlgse.to]
		mov     eax, dword [sv.addrs.addr_mute]
		mov     dword [eax], 0
		jmp     dword [sv.jcs.dlgse.next]
	dlgse_sora_over:
		jmp     dword [sv.jcs.dlgse.to]
	
	/////tits
	dlgse_tits:
		test    eax, eax
		jne     dlgse_tits_to
		
		mov     eax, dword [sv.addrs.addr_mute]
		cmp     dword [eax], 0
		jne     short dlgse_tits_to
		
		ret

	dlgse_tits_to:
		push    dword [sv.jcs.dlgse.to]
		ret     4
	}
}

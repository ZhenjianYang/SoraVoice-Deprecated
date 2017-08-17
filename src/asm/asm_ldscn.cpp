#include "asm_callee.h"
#include <SVData.h>

void ldscn() {
	_asm {
		sub     esp, 0x10
		push    ebx
		push    ebp
		push    esi
		mov     esi, [esp+0x20]
		push    edi
		
		push    eax
		push    ecx
		push    edx
		
		push    dword [sv.addrs.addr_ppscn]
		push    eax
		push    dword [esp + 0x38]
		call    dword [ASM::LoadScns]
		test    eax, eax
		
		pop     edx
		pop     ecx
		pop     eax
		
		jne     ldscn_iscn

	ldscn_call_ori:
		jmp     dword [sv.jcs.ldscn.next]

	ldscn_iscn:
		jmp     dword [sv.addrs.addr_iscn]
	}
}


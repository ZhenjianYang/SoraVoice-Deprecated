#include "asm.h"
#include "asm_callee.h"
#include <SVData.h>

__declspec(naked) void ASM::ldscn() {
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
		
		push    dword ptr [sv.addrs.addr_ppscn]
		push    eax
		push    dword ptr [esp + 0x38]
		call    dword ptr [ASM::LoadScns]
		test    eax, eax
		
		pop     edx
		pop     ecx
		pop     eax
		
		jne     ldscn_iscn

	//ldscn_call_ori:
		jmp     dword ptr [sv.jcs.ldscn.next]

	ldscn_iscn:
		jmp     dword ptr [sv.addrs.addr_iscn]
	}
}


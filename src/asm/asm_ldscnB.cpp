#include "asm_callee.h"
#include <SVData.h>

void ldscnB() {
	_asm {
		mov     edx, dword [esp + 8]
		mov     eax, edx
		shr     eax, 16

		cmp     eax, 1
		je      ldscnB_call
		cmp     eax, 0x21
		jne     ldscnB_call_ori
	ldscnB_call:
		mov     ecx, dword [esp + 4]
		push    ecx
		mov     dword [ecx], 0
		and     edx, 0xFFFF
		mov     ecx, dword [sv.addrs.addr_ppscn]
		mov     ecx, dword [eax * 4 + ecx]
		lea     eax, [edx * 8 + edx]
		lea     eax, [ecx + eax * 4]
		push    eax
		call    dword ASM::LoadScn
		test    eax, eax
		jz      ldscnB_call_ori

	ldscnB_return:
		ret

	ldscnB_call_ori:
		jmp     dword [sv.jcs.ldscn.to]
	}
}

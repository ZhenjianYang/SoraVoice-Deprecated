#include "asm_callee.h"
#include <SVData.h>

void aup() {
	_asm {
		cmp     dword [sv.za], 0
		je      aup_start
		
		cmp     byte [sv.status.end], 0
		jne     short aup_start

		cmp     dword [sv.status.startup], 0
		jne     short aup_start

		push    eax
		push    ecx
		push    edx
		call    dword [ASM::Init]
		pop     edx
		pop     ecx
		pop     eax

	aup_start:
		call    dword [sv.jcs.aup.to]

		cmp     dword [sv.order.autoPlay], 0
		je      short aup_return
		mov     dword [sv.order.autoPlay], 0

		or      al, 0x20

	aup_return:
		jmp    dword [sv.jcs.aup.next]
	}
}

#include "asm.h"
#include "inline_asm.h"
#include "Calls.h"
#include <SVData.h>

NACKED void ASM::aup() {
	INLINE_ASM(
		cmp     dword ptr [sv.za], 0;
		je      aup_start;

		cmp     dword ptr [sv.status.ended], 0;
		jne     short aup_start;

		cmp     dword ptr [sv.status.startup], 0;
		jne     short aup_start;

		push    eax;
		push    ecx;
		push    edx;
		call    ASM_Init;
		pop     edx;
		pop     ecx;
		pop     eax;

	aup_start:
		call    dword ptr [sv.jcs.aup.to];

		cmp     dword ptr [sv.order.autoPlay], 0;
		je      short aup_return;
		mov     dword ptr [sv.order.autoPlay], 0;

		or      al, 0x20;

	aup_return:
		jmp    dword ptr [sv.jcs.aup.next];
	);
}

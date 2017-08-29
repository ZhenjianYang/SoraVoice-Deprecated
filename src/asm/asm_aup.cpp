#include "asm.h"
#include "inline_asm.h"
#include "Calls.h"
#include <SVData.h>

NACKED void ASM::aup() {
	INLINE_ASM {
		cmp     dword ptr[SV.series], SVData::SERIES_ZEROAO;
		jne     aup_start;

		cmp     dword ptr[SV.status.ended], 0;
		jne     short aup_start;

		cmp     dword ptr[SV.status.startup], 0;
		jne     short aup_start;

		push    eax;
		push    ecx;
		push    edx;
		call    ASM_Init;
		pop     edx;
		pop     ecx;
		pop     eax;

	aup_start:
		call    dword ptr[SV.jcs.aup.to];

		cmp     dword ptr[SV.order.autoPlay], 0;
		je      short aup_return;
		mov     dword ptr[SV.order.autoPlay], 0;

		or al, 0x20;

	aup_return:
		jmp    dword ptr[SV.jcs.aup.next];
	}
}

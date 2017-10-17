#include "asm.h"
#include "inline_asm.h"
#include "Calls.h"
#include <SVData.h>

NACKED void ASM::dududu() {
	INLINE_ASM{
		cmp     dword ptr[SV.series], SERIES_SORA;
		je      dududu_sora;

		cmp     dword ptr[SV.series], SERIES_TITS;
		je      dududu_tits;


	//dududu_zero:
		cmp     dword ptr[SV.order.disableDududu], 0;
		je      dududu_zero_to;
		mov     dword ptr[esp + 0x10], 0;
	dududu_zero_to:
		jmp     dword ptr[SV.jcs.dududu.to];


	dududu_sora:
		cmp     dword ptr[SV.order.disableDududu], 0;
		je      short dududu_sora_over;

		mov     eax, dword ptr[SV.addrs.p_mute];
		cmp     byte ptr[eax], 0;
		jne     short dududu_sora_over;

		mov     byte ptr[eax], 1;
		add     esp, 4;
		call    dword ptr[SV.jcs.dududu.to];
		mov     eax, dword ptr[SV.addrs.p_mute];
		mov     byte ptr[eax], 0;
		jmp     dword ptr[SV.jcs.dududu.next];
	dududu_sora_over:
		jmp     dword ptr[SV.jcs.dududu.to];


	dududu_tits:
		cmp     dword ptr[SV.order.disableDududu], 0;
		jne     short dududu_tits_to;

		mov     eax, dword ptr[SV.addrs.p_mute];
		cmp     byte ptr[eax], 0;
		jne     short dududu_tits_to;

		ret;

	dududu_tits_to:
		push    dword ptr[SV.jcs.dududu.to];
		ret     4;
	}
}

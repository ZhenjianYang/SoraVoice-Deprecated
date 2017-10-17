#include "asm.h"
#include "inline_asm.h"
#include "Calls.h"
#include <SVData.h>

NACKED void ASM::dlgse() {
	INLINE_ASM{
		push    dword ptr[SV.order.disableDialogSE];

		push    ecx;
		push    edx;
		call    ASM_Stop;
		pop     edx;
		pop     ecx;

		pop     eax;

		cmp     dword ptr[SV.series], SERIES_SORA;
		je      dlgse_sora;
		cmp     dword ptr[SV.series], SERIES_TITS;
		je      dlgse_tits;


	//dlgse_zero:
		test    eax, eax;
		je      dlgse_zero_to;
		mov     dword ptr[esp + 0x10], 0;
	dlgse_zero_to:
		jmp     dword ptr[SV.jcs.dlgse.to];


	dlgse_sora:
		test    eax, eax;
		je      short dlgse_sora_over;

		mov     eax, dword ptr[SV.addrs.p_mute];
		cmp     byte ptr[eax], 0;
		jne     short dlgse_sora_over;

		mov     byte ptr[eax], 1;
		add     esp, 4;
		call    dword ptr[SV.jcs.dlgse.to];
		mov     eax, dword ptr[SV.addrs.p_mute];
		mov     byte ptr[eax], 0;
		jmp     dword ptr[SV.jcs.dlgse.next];
	dlgse_sora_over:
		jmp     dword ptr[SV.jcs.dlgse.to];


	dlgse_tits:
		test    eax, eax;
		jne     dlgse_tits_to;

		mov     eax, dword ptr[SV.addrs.p_mute];
		cmp     byte ptr[eax], 0;
		jne     short dlgse_tits_to;

		ret;

	dlgse_tits_to:
		push    dword ptr[SV.jcs.dlgse.to];
		ret     4;
	}
}

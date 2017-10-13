#include "asm.h"
#include "inline_asm.h"
#include "Calls.h"
#include <SVData.h>

NACKED void ASM::prst() {
	INLINE_ASM {
		pop     edx;

		cmp     dword ptr[SV.series], SERIES_ZEROAO;
		je      short prst_start;

	//prst_ed6:
		xor     ecx, ecx;
		push    ecx;
		push    ecx;

	prst_start:
		push    eax;
		push    edx;

		mov     ecx, dword ptr[eax]

		cmp     dword ptr[SV.dxver], DX8;
		je      short prst_dx8;

	//prst_dx9:
		push    dword ptr[ecx + 0x44];
		jmp     short prst_call;

	prst_dx8:
		push    dword ptr[ecx + 0x3C];

	prst_call:
		push    eax;
		call    ASM_Show;

		ret;
	}
}

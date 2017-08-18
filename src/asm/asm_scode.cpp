#include "asm.h"
#include "inline_asm.h"
#include "Calls.h"
#include <SVData.h>

NACKED void ASM::scode() {
	INLINE_ASM{
		push    eax;

		cmp     dword ptr[SV.tits], 0;
		je      scode_start;
		mov     eax, edx;

	scode_start:
		cmp     byte ptr[SV.scode.TEXT], al;
		je      record_code;
		cmp     byte ptr[SV.scode.SAY], al;
		je      record_code;
		cmp     byte ptr[SV.scode.TALK], al;
		je      record_code;
		cmp     byte ptr[SV.scode.MENU], al;
		je      record_code;
		cmp     byte ptr[SV.scode.MENUEND], al;
		jne     scode_return;

	record_code:
		movzx   eax, al;
		mov     dword ptr[SV.status.scode], eax;
		mov     dword ptr[SV.status.wait], 0;
		mov     dword ptr[SV.status.waitv], 0;
		mov     dword ptr[SV.rcd.count_ch], 0;

	scode_return:
		pop     eax;

		cmp     dword ptr[SV.sora], 0;
		jne     scode_sora_return;

		cmp     dword ptr[SV.tits], 0;
		jne     scode_tits_return;

	//scode_zero_return:
		mov     esi, esp;
		mov     eax, dword ptr[ebp + 8];
		jmp     dword ptr[SV.jcs.scode.next];

	scode_tits_return:
		push    esi;
		mov     ecx, edi;
		call    eax;
		jmp     dword ptr[SV.jcs.scode.next];

	scode_sora_return:
		push    edx;
		mov     ecx, edi;
		call    esi;
		jmp     dword ptr[SV.jcs.scode.next];
	}
}

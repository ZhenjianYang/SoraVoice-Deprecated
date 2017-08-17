#include "asm_callee.h"
#include <SVData.h>

void scode() {
	_asm {
		push    eax

		cmp     dword ptr [sv.tits], 0
		je      scode_start
		mov     eax, edx

	scode_start:
		cmp     byte ptr [sv.scode.TEXT], al
		je      record_code
		cmp     byte ptr [sv.scode.SAY], al
		je      record_code
		cmp     byte ptr [sv.scode.TALK], al
		je      record_code
		cmp     byte ptr [sv.scode.MENU], al
		je      record_code
		cmp     byte ptr [sv.scode.MENUEND], al
		jne     scode_return

	record_code:
		movzx   eax, al
		mov     dword ptr [sv.status.scode], eax

	scode_return:
		pop     eax

		cmp     dword ptr [sv.sora], 0
		jne     scode_sora_return
		
		cmp     dword ptr [sv.tits], 0
		jne     scode_tits_return
		
	//scode_zero_return:
		mov     esi, esp
		mov     eax, dword ptr [ebp + 8]
		jmp     dword ptr [sv.jcs.scode.next]

	scode_tits_return:
		push    esi
		mov     ecx, edi
		call    eax
		jmp     dword ptr [sv.jcs.scode.next]
		
	scode_sora_return:
		push    edx
		mov     ecx, edi
		call    esi
		jmp     dword ptr [sv.jcs.scode.next]
	}
}

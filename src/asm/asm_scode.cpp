#include "asm_callee.h"
#include <SVData.h>

void scode() {
	_asm {
		push    eax

		cmp     dword [sv.tits], 0
		je      scode_start
		mov     eax, edx

	scode_start:
		cmp     byte [sv.scode.TEXT], al
		je      record_code
		cmp     byte [sv.scode.SAY], al
		je      record_code
		cmp     byte [sv.scode.TALK], al
		je      record_code
		cmp     byte [sv.scode.MENU], al
		je      record_code
		cmp     byte [sv.scode.MENUEND], al
		jne     scode_return

	record_code:
		movzx   dword [sv.status.scode], al

	scode_return:
		pop     eax

		cmp     dword [sv.sora], 0
		jne     scode_sora_return
		
		cmp     dword [sv.tits], 0
		jne     scode_tits_return
		
	scode_zero_return
		mov     esi, esp
		mov     eax, [ebp+08]
		jmp     dword [sv.jcs.scode.next]

	scode_tits_return:
		push    esi
		mov     ecx, edi
		call    eax
		jmp     dword [sv.jcs.scode.next]
		
	%scode_sora_return:
		push    edx
		mov     ecx, edi
		call    esi
		jmp     dword [sv.jcs.scode.next]
	}
}

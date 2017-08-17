#include "asm_callee.h"
#include <SVData.h>

void text() {
	_asm {
		push    eax
		
		cmp     dword [sv.sora], 0
		jne     text_sora
		cmp     dword [sv.tits], 0
		je      text_start
		
		mov     eax, ebx //tits
		jmp     short text_start
	text_sora:
		mov     eax, edx //sora

	text_start:
		cmp     byte [eax], 0x20
		jb      jcode

		cmp     byte [eax], '#'
		jnz     short checkcode

		push    eax
	loop:
		inc     eax
		cmp     byte [eax], '0'
		jb      short loopend
		cmp     byte [eax], '9'
		jbe     short loop

	loopend:
		cmp     byte [eax], 'A'
		jnz     checkV
		mov     dword [sv.status.scode], 0
		pop     eax
		jmp     short text_return

	checkV:
		cmp     byte [eax], 'v'
		pop     eax
		jnz     short text_return

		push    ecx
		push    edx
		push    eax
		call    ASM::Play
		pop     edx
		pop     ecx

	text_return:
		pop     eax
		push    dword [sv.jcs.text.to]
		ret     4

	checkcode:
		mov     eax, dword [sv.status.scode]
		cmp     eax, dword [sv.scode.TEXT]
		je      checkwait
		cmp     eax, dword [sv.scode.SAY]
		je      checkwait
		cmp     eax, dword [sv.scode.TALK]
		jne     short text_return

	checkwait:
		cmp     dword [sv.status.wait], 0
		je      count
		mov     dword [sv.status.wait], 0
		mov     dword [sv.rcd.count_ch], 0
	count:
		add     dword [sv.rcd.count_ch], 1
		cmp     dword [sv.rcd.count_ch], 1
		jne     short text_return
		mov     eax, dword [sv.rcd.now]
		mov     dword [sv.rcd.time_textbeg], eax
		jmp     short text_return

	jcode:
		cmp     byte [eax], 2
		jne     short text_returnb
		cmp     dword [sv.rcd.count_ch], 0
		je      short text_returnb
		mov     dword [sv.status.wait], 1

	text_returnb:
		pop     eax
		ret
	}
}

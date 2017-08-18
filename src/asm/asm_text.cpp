#include "asm.h"
#include "inline_asm.h"
#include "Calls.h"
#include <SVData.h>

NACKED void ASM::text() {
	INLINE_ASM{
		push    eax;

		cmp     dword ptr[SV.sora], 0;
		jne     text_sora;
		cmp     dword ptr[SV.tits], 0;
		je      text_start;

		mov     eax, ebx;
		jmp     short text_start;
	text_sora:
		mov     eax, edx;

	text_start:
		cmp     byte ptr[eax], 0x20;
		jb      jcode;

		cmp     byte ptr[eax], '#';
		jnz     short checkcode;

		push    eax;
	loopstart:
		inc     eax;
		cmp     byte ptr[eax], '0';
		jb      short loopend;
		cmp     byte ptr[eax], '9';
		jbe     short loopstart;

	loopend:
		cmp     byte ptr[eax], 'A';
		jnz     checkV;
		mov     dword ptr[SV.status.scode], 0;
		pop     eax;
		jmp     short text_return;

	checkV:
		cmp     byte ptr[eax], 'v';
		pop     eax;
		jnz     short text_return;

		push    ecx;
		push    edx;
		push    eax;
		call    ASM_Play;
		pop     edx;
		pop     ecx;

	text_return:
		pop     eax;
		push    dword ptr[SV.jcs.text.to];
		ret     4;

	checkcode:
		mov     eax, dword ptr[SV.status.scode];
		cmp     eax, dword ptr[SV.scode.TEXT];
		je      checkwait;
		cmp     eax, dword ptr[SV.scode.SAY];
		je      checkwait;
		cmp     eax, dword ptr[SV.scode.TALK];
		jne     short text_return;

	checkwait:
		cmp     dword ptr[SV.status.wait], 0;
		je      count;
		mov     dword ptr[SV.status.wait], 0;
		mov     dword ptr[SV.rcd.count_ch], 0;
	count:
		add     dword ptr[SV.rcd.count_ch], 1;
		cmp     dword ptr[SV.rcd.count_ch], 1;
		jne     short text_return;
		mov     eax, dword ptr[SV.rcd.now];
		mov     dword ptr[SV.rcd.time_textbeg], eax;
		jmp     short text_return;

	jcode:
		cmp     byte ptr[eax], 2;
		jne     short text_returnb;
		cmp     dword ptr[SV.rcd.count_ch], 0;
		je      short text_returnb;
		mov     dword ptr[SV.status.wait], 1;

	text_returnb:
		pop     eax;
		ret;
	}
}

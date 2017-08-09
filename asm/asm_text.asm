%include "macro_common"

%define tmp _tmp(0)
%define vs vs_text

%ifdef za
%define eadx eax
%define acl cl
%else
%define eadx edx
%define acl al
%endif

[BITS 32]
section text vstart=vs
	push    eax
	push    ebx
	call    text_start
text_start:
	pop     ebx
	pop     eax
	sub     ebx, 7 + vs
	mov     dword [ebx + tmp], eax
	;pop     eax

	;push    eax
%ifdef sora
	mov     eax, edx
%elifdef za
	mov     eax, dword [esp]
;%elifdef tits
;	mov     eax, dword [ebx + tmp]
%endif
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
	mov     byte [ebx + status_scode], 0
	pop     eax
	jmp     short text_return

checkV:
	cmp     byte [eax], 'v'
	pop     eax
	jnz     short text_return

	push    ecx
	push    edx
	push    ebx + ptr_initparam
	push    eax
	call    dword [ebx + voice_play]
	pop     edx
	pop     ecx

text_return:
	pop     eax
	push    dword [ebx + to(jcs_text)]
	mov     ebx, dword [ebx + tmp]
	ret

checkcode:
	mov     al, byte [ebx + status_scode]
	cmp     al, byte [ebx + scode_TEXT]
	je      checkwait
	cmp     al, byte [ebx + scode_SAY]
	je      checkwait
	cmp     al, byte [ebx + scode_TALK]
	jne     text_return

checkwait:
	cmp     byte [ebx + status_wait], 0
	je      count
	mov     byte [ebx + status_wait], 0
	mov     dword [ebx + ptr_cnt], 0
count:
	cmp     dword [ebx + ptr_cnt], 0
	jne     notfirst
	mov     eax, dword [ebx + ptr_now]
	mov     dword [ebx + ptr_ttb], eax
	mov     dword [ebx + ptr_cnt], 1
	jmp     text_return

notfirst:
	mov     eax, dword [ebx + ptr_cnt]
	inc     eax
	mov     dword [ebx + ptr_cnt], eax
	jmp     text_return

jcode:
	cmp     byte [eax], 2
	jne     short text_returnb
	cmp     dword [ebx + ptr_cnt], 0
	je      short text_returnb
	mov     byte [ebx + status_wait], 1

text_returnb:
	pop     eax
	push    dword [ebx + next(jcs_text)]
	mov     ebx, dword [ebx + tmp]
	ret

%undef tmp
%undef vs

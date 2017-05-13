%include "macro_common"

%define tmp _tmp(4)
%define vs vs_scode 

[BITS 32]
section scode vstart=vs
	push    eax
	push    ebx
	call    scode_start
scode_start:
	pop     ebx
	pop     eax
	sub     ebx, 7 + vs
	mov     dword [ebx + tmp], eax
	pop     eax

	cmp     byte [ebx + scode_TEXT], al
	je      record_code
	cmp     byte [ebx + scode_SAY], al
	je      record_code
	cmp     byte [ebx + scode_TALK], al
	je      record_code
	cmp     byte [ebx + scode_MENU], al
	je      record_code
	cmp     byte [ebx + scode_MENUEND], al
	je      record_code

;	cmp     byte [ebx + scode_CLSW], al
;	jne     scode_return
;	push    eax
;	push    ecx
;	push    edx
;	push    ebx + ptr_initparam
;	call    dword [ebx + voice_stop]
;	pop     edx
;	pop     ecx
;	pop     eax
	jmp     scode_return

record_code:
	mov     byte [ebx + status_scode], al

scode_return:
%ifdef za
	mov     esi, esp
	mov     eax, [ebp+08]
%else
	push    edx
	mov     ecx, edi
	call    esi
%endif

	push    dword [ebx + next(jcs_scode)]
	mov     ebx, dword [ebx + tmp]
	ret

%undef tmp
%undef vs

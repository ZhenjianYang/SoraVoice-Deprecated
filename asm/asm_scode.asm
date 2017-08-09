%include "macro_common"

%define tmp _tmp(4)
%define vs vs_scode

%ifdef tits
%define adl dl
%else
%define adl al
%endif

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

	cmp     byte [ebx + scode_TEXT], adl
	je      record_code
	cmp     byte [ebx + scode_SAY], adl
	je      record_code
	cmp     byte [ebx + scode_TALK], adl
	je      record_code
	cmp     byte [ebx + scode_MENU], adl
	je      record_code
	cmp     byte [ebx + scode_MENUEND], adl
	jne     scode_return

record_code:
	mov     byte [ebx + status_scode], adl

scode_return:
%ifdef za
	mov     esi, esp
	mov     eax, [ebp+08]
%elifdef tits
	push    esi
	mov     ecx, edi
	call    eax
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
%undef adl

%include "macro_common"

%define tmp tmp0
%define vs vs_text

%ifdef ao
%define eadx eax
%else
%define eadx edx
%endif

[BITS 32]
section text vstart=vs
	push    eax
	push    ebx
	call    text_start
text_start:
	pop     ebx
	pop     eax
	mov     dword [ebx + tmp], eax
	sub     ebx, 7 + vs
	pop     eax

	jb      jcode

	cmp     byte [eadx], '#'
	jnz     short text_return

	push    eadx
loop:
	inc     eadx
	cmp     byte [eadx], '0'
	jb      short loopend
	cmp     byte [eadx], '9'
	jbe     short loop

loopend:
	cmp     byte [eadx], 'V'
	pop     eadx
	jnz     short text_return

	push    eax
	push    ecx
	push    edx
	push    dword [ebx + ptr_initparam]
	push    eadx
	call    dword [ebx + voice_play]
	pop     edx
	pop     ecx
	pop     eax

text_return:
	push    dword [ebx + to(jcs_text)]
	mov     ebx, dword [ebx + tmp]
	ret

jcode:
	cmp     byte [eadx], 5
	jnz     jcode2
	mov     byte [ebx + status_code5], 1

jcode2:
	cmp     byte [eadx], 2
	jne     short text_returnb
	cmp     dword [ebx + ptr_cnt], 0
	je      short text_returnb
	mov     byte [ebx + status_wait], 1

text_returnb:
	push    dword [ebx + next(jcs_text)]
	mov     ebx, dword [ebx + tmp]
	ret

times 0x100-($-$$) db 0

%undef tmp
%undef vs

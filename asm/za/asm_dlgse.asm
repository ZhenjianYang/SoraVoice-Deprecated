%include "macro_common"

%define tmp tmp2
%define vs vs_dlgse 

[BITS 32]
section dlgse vstart=vs
	push    eax
	push    ebx
	call    dlgse_start
dlgse_start:
	pop     ebx
	pop     eax
	mov     dword [ebx + tmp], eax
	sub     ebx, 7 + vs
	pop     eax

	push    dword [ebx + ptr_initparam]
	call    dword [ebx + voice_stop]

callend:
	cmp     byte [ebx + order_dlgse], 0
	je      short dlgse_return

%ifdef ao
	mov     dword [esp + 0x0C], 0
	call    dword [ebx + to(order_dlgse)]
%else
	cmp     byte [ebx + addr_mute], 0
	jne     short dlgse_return

	mov     byte [ebx + addr_mute], 1
	call    dword [ebx + to(jcs_dlgse)]
	mov     byte [ebx + addr_mute], 0
%endif

	push    dword [ebx + next(jcs_dlgse)]
	mov     ebx, dword [ebx + tmp]
	ret

dlgse_return:
	call    dword [ebx + to(jcs_dlgse)]
	push    dword [ebx + next(jcs_dlgse)]
	mov     ebx, dword [ebx + tmp]
	ret

times 0x100-($-$$) db 0

%undef tmp
%undef vs

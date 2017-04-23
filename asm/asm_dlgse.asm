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
	sub     ebx, 7 + vs
	mov     dword [ebx + tmp], eax
	pop     eax

	push    eax
	push    ecx
	push    edx
	push    ebx + ptr_initparam
	call    dword [ebx + voice_stop]
	pop     edx
	pop     ecx
	pop     eax

callend:
	cmp     byte [ebx + order_dlgse], 0
	je      short dlgse_return

%ifdef za
	mov     dword [esp + 0x0C], 0
	call    dword [ebx + to(jcs_dlgse)]
%else
	mov     ecx, dword [ebx + addr_mute]
	cmp     byte [ecx], 0
	jne     short dlgse_return

	mov     byte [ecx], 1
	call    dword [ebx + to(jcs_dlgse)]
	mov     ecx, dword [ebx + addr_mute]
	mov     byte [ecx], 0
%endif

	push    dword [ebx + next(jcs_dlgse)]
	mov     ebx, dword [ebx + tmp]
	ret

dlgse_return:
	call    dword [ebx + to(jcs_dlgse)]
	push    dword [ebx + next(jcs_dlgse)]
	mov     ebx, dword [ebx + tmp]
	ret

%undef tmp
%undef vs

%include "macro_common"

%define tmp _tmp(2)
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
	;pop     eax
	
	;push    eax
	
	mov     al, byte [ebx + order_dlgse]
	push    eax
	
	push    ecx
	push    edx
	push    ebx + ptr_initparam
	call    dword [ebx + voice_stop]
	pop     edx
	pop     ecx
	
	pop     eax

	cmp     al, 0
	pop     eax
%ifdef tits
		jne     short tits_close_dlgse
		mov     eax, dword [ebx + addr_mute]
		cmp     byte [eax], 0
		je      short dlgse_return
	tits_close_dlgse:
		push    dword [ebx + to(jcs_dlgse)]
		mov     ebx, dword [ebx + tmp]
		ret
%else
		je      short close_dlgse_over
	%ifdef za
		mov     dword [esp + 0x0C], 0
	%else ;sora
		mov     eax, dword [ebx + addr_mute]
		cmp     byte [eax], 0
		jne     short close_dlgse_over

		mov     byte [eax], 1
		call    dword [ebx + to(jcs_dlgse)]
		mov     eax, dword [ebx + addr_mute]
		mov     byte [eax], 0
		jmp     dlgse_return
	%endif
	close_dlgse_over:
		call    dword [ebx + to(jcs_dlgse)]
%endif

dlgse_return:
	push    dword [ebx + next(jcs_dlgse)]
	mov     ebx, dword [ebx + tmp]
	ret

%undef tmp
%undef vs

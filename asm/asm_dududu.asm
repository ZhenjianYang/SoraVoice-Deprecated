%include "macro_common"

%define tmp _tmp(1)
%define vs vs_dududu

[BITS 32]
section dududu vstart=vs
	push    eax
	push    ebx
	call    dududu_start
dududu_start:
	pop     ebx
	pop     eax
	sub     ebx, 7 + vs
	mov     dword [ebx + tmp], eax
	pop     eax

	cmp     byte [ebx + order_dududu], 0
%ifdef tits
		jne     short tits_close_dududu
		mov     eax, dword [ebx + addr_mute]
		cmp     byte [eax], 0
		je      short dududu_return
	tits_close_dududu:
		push    dword [ebx + to(jcs_dududu)]
		mov     ebx, dword [ebx + tmp]
		ret
%else
		je      short close_dududu_over
	%ifdef za
		mov     dword [esp + 0x0C], 0
	%else ;sora
		mov     eax, dword [ebx + addr_mute]
		cmp     byte [eax], 0
		jne     short close_dududu_over

		mov     byte [eax], 1
		call    dword [ebx + to(jcs_dududu)]
		mov     eax, dword [ebx + addr_mute]
		mov     byte [eax], 0
		jmp     dududu_return
	%endif
	close_dududu_over:
		call    dword [ebx + to(jcs_dududu)]
%endif

dududu_return:
	push    dword [ebx + next(jcs_dududu)]
	mov     ebx, dword [ebx + tmp]
	ret

%undef tmp
%undef vs

%include "macro_common"

%define tmp tmp1
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

	cmp     byte [ebx + status_code5], 0
	je      code5end
	push    eax
	push    ecx
	push    edx
	push    ebx + ptr_initparam
	call    dword [ebx + voice_stop]
	pop     edx
	pop     ecx
	pop     eax

code5end:
	mov     al, byte [ebx + status_scode]
	cmp     byte [ebx + scode_TEXT], al
	je      count
	cmp     byte [ebx + scode_SAY], al
	je      count
	cmp     byte [ebx + scode_TALK], al
	je      count
	jmp     disabledududu

count:
	cmp     dword [ebx + ptr_cnt], 0
	je      first
	
	mov     eax, dword [ebx + ptr_cnt]
	inc     eax
	mov     dword [ebx + ptr_cnt], eax
	jmp     disabledududu
	
first:
	mov     eax, dword [ebx + ptr_now]
	mov     dword [ebx + ptr_ttb], eax
	mov     dword [ebx + ptr_cnt], 1

disabledududu:
	cmp     byte [ebx + order_dududu], 0
	je      short dududu_return

%ifdef za
	mov     dword [esp + 0x0C], 0
	call    dword [ebx + to(jcs_dududu)]
%else
	mov     ecx, dword [ebx + addr_mute]
	cmp     byte [ecx], 0
	jne     short dududu_return

	mov     byte [ecx], 1
	call    dword [ebx + to(jcs_dududu)]
	mov     ecx, dword [ebx + addr_mute]
	mov     byte [ecx], 0
%endif

	push    dword [ebx + next(jcs_dududu)]
	mov     ebx, dword [ebx + tmp]
	ret

dududu_return:
	call    dword [ebx + to(jcs_dududu)]
	push    dword [ebx + next(jcs_dududu)]
	mov     ebx, dword [ebx + tmp]
	ret

%undef tmp
%undef vs

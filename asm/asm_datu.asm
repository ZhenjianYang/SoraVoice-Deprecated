%include "macro_common"

%define tmp _tmp(6)
%define vs vs_datu

[BITS 32]
section datu vstart=vs
	push    eax
	push    ebx
	call    datu_start
datu_start:
	pop     ebx
	pop     eax
	sub     ebx, 7 + vs
	mov     dword [ebx + tmp], eax
	;pop    eax

	;push   eax
	;mov    eax, dword [ebx + tmp]
	pop     edx
	mov     edx, [edi]
	cmp     dword[edx + 4], RAW_FILE_MAGIC
	jne     datu_call_ori

	push    esi
	push    edi

	mov     edi, [eax]
	lea     esi, [edx + 12]
	mov     ecx, dword [edx + 8]
	rep movsb

	mov     dword [eax], edi
	pop     edi
	mov     dword [edi], esi
	pop     esi
	
	mov     eax, dword [edx + 8]
	jmp     datu_return

datu_call_ori:
	call    dword [ebx + to(jcs_datu)]

datu_return:
	push    dword [ebx + next(jcs_datu)]
	mov     ebx, dword [ebx + tmp]
	ret

%undef tmp
%undef vs

;Reference: https://github.com/Ouroboros/JuusanKoubou/tree/master/Source/Falcom/ED6FC%20Steam

%include "macro_common"

%define tmp _tmp(5)
%define vs vs_ldscn

[BITS 32]
section ldscn vstart=vs

%ifdef tits

	push    eax
	push    ebx
	call    ldscn_start
ldscn_start:
	pop     ebx
	pop     eax
	sub     ebx, 7 + vs
	mov     dword [ebx + tmp], eax
	pop     eax
	
	sub     esp,0x10
	push    dword [ebx + tmp]
	push    ebp
	push    esi
	mov     esi,[esp+0x20]
	push    edi
	
	push    eax
	push    ecx
	push    edx
	
	push    dword [ebx + addr_ppscn]
	push    eax
	push    dword [esp + 0x38]
	call    dword [ebx + voice_ldscns]
	test    eax, eax
	
	pop     edx
	pop     ecx
	pop     eax
	
	jne     ldscn_iscn

ldscn_call_ori:
	push    dword [ebx + next(jcs_ldscn)]
	mov     ebx, dword [ebx + tmp]
	ret

ldscn_iscn:
	push    dword [ebx + addr_iscn]
	mov     ebx, dword [ebx + tmp]
	ret

%endif

%undef tmp
%undef vs

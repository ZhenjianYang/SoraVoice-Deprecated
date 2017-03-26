%ifdef sora_3rd
%include "macro_3rd"
%elifdef sora_sc
%include "macro_sc"
%else
%include "macro_fc"
%endif

[BITS 32]
section disable_du vstart=vs_disable_du
	push    eax
	mov     al, byte [ptr_flag_disable_du]
	test    al, al
	je      short return
	nop
	mov     eax, dword [ptr_volume]
	mov     dword [ptr_tmp], eax
	mov     eax, dword [ptr_volume0]
	mov     dword [ptr_tmp + 4], eax
	mov     dword [ptr_volume], 0
	mov     dword [ptr_volume0], volume_lowest
	pop     eax
	call    addr_dududu
	push    eax
	mov     eax, dword [ptr_tmp]
	mov     dword [ptr_volume], eax
	mov     eax, dword [ptr_tmp + 4]
	mov     dword [ptr_volume0], eax
	pop     eax
	jmp     addr_call_dududu+5
	nop
return:
	pop     eax
	call    addr_dududu
	jmp     addr_call_dududu+5
	
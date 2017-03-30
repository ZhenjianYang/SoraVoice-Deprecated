%include "macro_common"

%ifdef sora_3rd
%include "macro_3rd"
%elifdef sora_sc
%include "macro_sc"
%else
%include "macro_fc"
%endif

[BITS 32]
section dlgse vstart=vs_dlgse
	push    eax
	
	mov     al, byte [ptr_flag_skipvoice]
	test    al, al
	je      short callend
	
	push    ebx
	push    ecx
	push    edx
	push    ptr_initparam
	call    dword [ptr_voice_stop]
	pop     edx
	pop     ecx
	pop     ebx
	
callend:
	mov     al, byte [ptr_flag_disable_dlgse]
	mov     byte [ptr_flag_disable_dlgse], 0
	test    al, al
	je      short return

	mov     eax, dword [ptr_volume]
	mov     dword [ptr_tmp], eax
	mov     eax, dword [ptr_volume0]
	mov     dword [ptr_tmp + 4], eax
	mov     dword [ptr_volume], 0
	mov     dword [ptr_volume0], volume_lowest
	
	pop     eax
	call    addr_dlgse
	
	push    eax
	mov     eax, dword [ptr_tmp]
	mov     dword [ptr_volume], eax
	mov     eax, dword [ptr_tmp + 4]
	mov     dword [ptr_volume0], eax
	pop     eax
	
	jmp     addr_call_dlgse+5
	
return:
	pop     eax
	call    addr_dlgse
	jmp     addr_call_dlgse+5

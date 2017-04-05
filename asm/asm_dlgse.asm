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

	cmp     byte [ptr_flag_skipvoice], 0
	je      short callend

	push    ptr_initparam
	call    dword [ptr_voice_stop]

callend:
	cmp     byte [ptr_flag_disable_dlgse], 0 
	je      short return
	mov     byte [ptr_flag_disable_dlgse], 0

	cmp     byte [ptr_mute], 0
	jne     short return

	mov     byte [ptr_mute], 1
	call    addr_dlgse
	mov     byte [ptr_mute], 0

	jmp     addr_call_dlgse+5

return:
	call    addr_dlgse
	jmp     addr_call_dlgse+5

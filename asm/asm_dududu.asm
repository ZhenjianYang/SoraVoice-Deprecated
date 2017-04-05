%include "macro_common"

%ifdef sora_3rd
%include "macro_3rd"
%elifdef sora_sc
%include "macro_sc"
%else
%include "macro_fc"
%endif

[BITS 32]
section dududu vstart=vs_dududu

	cmp     byte [ptr_flag_disable_du], 0
	je      short return
	
	cmp     byte [ptr_mute], 0
	jne     short return

	mov     byte [ptr_mute], 1
	call    addr_dududu
	mov     byte [ptr_mute], 0

	jmp     addr_call_dududu+5

return:
	call    addr_dududu
	jmp     addr_call_dududu+5

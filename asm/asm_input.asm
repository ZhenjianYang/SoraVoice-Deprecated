%include "macro_common"

%ifdef sora_3rd
%include "macro_3rd"
%elifdef sora_sc
%include "macro_sc"
%else
%include "macro_fc"
%endif

[BITS 32]
section input vstart=vs_input
	cmp byte [ptr_flag_ended], 0
	jne short return
	
	push ecx

	cmp dword [ptr_sv], 0
	jne short check_input
	
	push edx
	push ptr_initparam
	call [ptr_voice_init]
	pop ebx

check_input:
	mov eax, [ptr_p_keys]
	add eax, dik_beg
	mov cl, dik_end - dik_beg + 1
	
loop_check_input:
	mov ch, byte [eax]
	test ch, ch
	jnz short input_found
	inc eax
	dec cl
	jnz short loop_check_input
	mov eax, ptr_keys_old
	mov dword [ptr_keys_old], 0
	mov dword [ptr_keys_old+4], 0
	jmp short auto_play
	
input_found:
	push edx
	push ptr_initparam
	call [ptr_voice_input]
	pop edx

auto_play:
	pop ecx

	cmp byte [ptr_flag_autoplay], 0
	je short return
	
	mov byte [ptr_flag_autoplay], 0
	mov eax, [ptr_p_keys]
	mov byte [eax + dik_space], dik_press

return:
	jmp addr_jmpto_input



set path=.\nasm

set out=..\bin\asm
md %out%

nasm -o %out%\fc_050_playvoice -Dsora_fc asm_playvoice.asm
nasm -o %out%\fc_0C0_disabledu -Dsora_fc asm_disabledu.asm
nasm -o %out%\fc_140_dlgse -Dsora_fc asm_dlgse.asm

nasm -o %out%\sc_050_playvoice -Dsora_sc asm_playvoice.asm
nasm -o %out%\sc_0C0_disabledu -Dsora_sc asm_disabledu.asm
nasm -o %out%\sc_140_dlgse -Dsora_sc asm_dlgse.asm

nasm -o %out%\3rd_050_playvoice -Dsora_3rd asm_playvoice.asm
nasm -o %out%\3rd_0C0_disabledu -Dsora_3rd asm_disabledu.asm
nasm -o %out%\3rd_140_dlgse -Dsora_3rd asm_dlgse.asm


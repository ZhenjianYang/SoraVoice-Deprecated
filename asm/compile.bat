set path=.\nasm

set out=..\bin\asm
md %out%

for %%i in (fc sc 3rd) do (
nasm -o %out%\%%i_240_text -Dsora_%%i asm_text.asm
nasm -o %out%\%%i_2C0_dududu -Dsora_%%i asm_dududu.asm
nasm -o %out%\%%i_340_dlgse -Dsora_%%i asm_dlgse.asm
nasm -o %out%\%%i_400_input -Dsora_%%i asm_input.asm
)



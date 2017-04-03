set path=.\nasm

set out=..\bin\asm
md %out%

for %%i in (fc sc 3rd) do (
nasm -o %out%\%%i_text -Dsora_%%i asm_text.asm
nasm -o %out%\%%i_dududu -Dsora_%%i asm_dududu.asm
nasm -o %out%\%%i_dlgse -Dsora_%%i asm_dlgse.asm
nasm -o %out%\%%i_input -Dsora_%%i asm_input.asm
nasm -o %out%\%%i_d3d -Dsora_%%i asm_d3d.asm
)



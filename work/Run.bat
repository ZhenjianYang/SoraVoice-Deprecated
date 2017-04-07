set nasm=nasm\nasm.exe
set dir_asm=..\asm
set dir_main=main
set dir_bin=bin
set dir_new=new

md %dir_new%
md %dir_bin%

for %%i in (fc sc 3rd) do (
%nasm% -i%dir_asm%\ -o %dir_bin%\%%i_text -Dsora_%%i %dir_asm%\asm_text.asm
%nasm% -i%dir_asm%\ -o %dir_bin%\%%i_dududu -Dsora_%%i %dir_asm%\asm_dududu.asm
%nasm% -i%dir_asm%\ -o %dir_bin%\%%i_dlgse -Dsora_%%i %dir_asm%\asm_dlgse.asm
%nasm% -i%dir_asm%\ -o %dir_bin%\%%i_input -Dsora_%%i %dir_asm%\asm_input.asm
%nasm% -i%dir_asm%\ -o %dir_bin%\%%i_wait -Dsora_%%i %dir_asm%\asm_wait.asm

Importer_EXE %dir_new%\%%i_voice.exe  %dir_main%\%%i_main.exe -r zrep_%%i.txt -m %dir_asm%\macro_%%i -b %dir_bin%\%%i_text %dir_bin%\%%i_dududu %dir_bin%\%%i_dlgse %dir_bin%\%%i_input %dir_bin%\%%i_wait
)
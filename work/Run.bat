set nasm=nasm\nasm.exe
set dir_asm=..\asm
set dir_main=main
set dir_bin=bin
set dir_new=new

md %dir_new%
md %dir_bin%

setlocal enabledelayedexpansion

for %%i in (fc sc 3rd) do (

for %%j in (%dir_asm%\*.asm) do (
set p=%%~nj
%nasm% -i%dir_asm%\ -o %dir_bin%\%%i_!p:asm_=! -Dsora_%%i %%j
)

Importer_EXE %dir_new%\%%i_voice.exe  %dir_main%\%%i_main.exe -r zrep_%%i.txt -m %dir_asm%\macro_%%i -b %dir_bin%\%%i_*
)
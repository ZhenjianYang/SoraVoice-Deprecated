set nasm=nasm\nasm.exe
set dir_asm=..\asm
set dir_main=main
set dir_bin=bin
set dir_new=new
set data=..\example\SoraData.ini
set input=asm_all

md %dir_new%
md %dir_bin%

setlocal enabledelayedexpansion

for %%i in (sora za) do (
%nasm% -i%dir_asm%\ -o %dir_bin%\%%i_!input:asm_=! -D%%i %dir_asm%\%input%.asm
)


for %%i in (fc sc 3rd) do (
Importer_EXE %dir_new%\%%i_voice.exe  %dir_main%\%%i_main.exe %data% %dir_bin%\sora_!input:asm_=!
)
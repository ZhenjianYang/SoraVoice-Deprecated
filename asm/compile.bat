set nasm=nasm\nasm.exe
set dir_asm=.
set dir_bin=bin
set dir_rc=..\src\rc_hk

set input=asm_all

md %dir_bin%

setlocal enabledelayedexpansion

for %%i in (sora za tits) do (
%nasm% -i%dir_asm%\ -o %dir_bin%\%%i_!input:asm_=! -D%%i %dir_asm%\%input%.asm
copy /Y %dir_bin%\%%i_!input:asm_=! %dir_rc%\
)



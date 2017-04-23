set nasm=nasm\nasm.exe
set dir_asm=.
set dir_bin=bin

set input=asm_all

md %dir_bin%

for %%i in (sora za) do (
%nasm% -i%dir_asm%\ -o %dir_bin%\%%i_!input:asm_=! -D%%i %dir_asm%\%input%.asm
)



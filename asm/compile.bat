set nasm=nasm\nasm.exe
set dir_asm=.
set dir_bin=bin

md %dir_bin%

setlocal enabledelayedexpansion

for %%i in (fc sc 3rd) do (

for %%j in (%dir_asm%\*.asm) do (
set p=%%~nj
%nasm% -i%dir_asm%\ -o %dir_bin%\%%i_!p:asm_=! -Dsora_%%i %%j
)

)



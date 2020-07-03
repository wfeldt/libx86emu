echo off 
SET PATH=%PATH%;C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.16.27023\bin
echo on

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.BAT" 

del libx86emu.lib 
cl.exe /D_USRDLL /D_WINDLL api.c decode.c mem.c ops.c ops2.c prim_ops.c /link /DLL /OUT:libx86emu.dll
if ERRORLEVEL 1 pause
del *.obj




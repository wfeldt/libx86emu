echo off 
SET PATH=%PATH%;C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.16.27023\bin
echo on

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.BAT" 

cl.exe x86emu-demo.c ..\libx86emu.lib
if ERRORLEVEL 1 pause
del *.obj 


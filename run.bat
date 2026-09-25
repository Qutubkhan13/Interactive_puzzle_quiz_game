@echo off
rem game.exe needs freeglut.dll, which lives in the MSYS2 UCRT64 bin folder
set "PATH=C:\msys64\ucrt64\bin;%PATH%"
game.exe

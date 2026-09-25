@echo off
setlocal
rem MSYS2 UCRT64 toolchain (install FreeGLUT with:
rem   pacman -S --needed mingw-w64-ucrt-x86_64-freeglut  in an MSYS2 UCRT64 terminal)
set "MSYS=C:\msys64\ucrt64"
set "PATH=%MSYS%\bin;%PATH%"
g++ main.cpp -o game.exe -I"%MSYS%\include" -L"%MSYS%\lib" -lfreeglut -lopengl32 -lglu32
if errorlevel 1 (
    echo Build failed.
    exit /b 1
)
echo Build succeeded. Run with: run.bat   or   game.exe

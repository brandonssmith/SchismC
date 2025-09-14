@echo off
echo Building SchismC Console Demo Application...
echo.

echo Assembling with MASM...
ml64 /c console_demo.asm
if errorlevel 1 (
    echo ERROR: Assembly failed!
    pause
    exit /b 1
)

echo Linking with LINK...
link /subsystem:console console_demo.obj /out:console_demo.exe
if errorlevel 1 (
    echo ERROR: Linking failed!
    pause
    exit /b 1
)

echo.
echo SUCCESS: Console demo application built!
echo Running console_demo.exe...
echo.

console_demo.exe

echo.
echo Demo completed!
pause


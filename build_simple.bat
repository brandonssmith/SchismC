@echo off
REM Simple SchismC Build Script for Windows

echo Building SchismC...

REM Set up Microsoft Build Tools environment
set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64"
set "PATH=%VS_PATH%;%PATH%"

echo Testing tools...

REM Test GCC
gcc --version
if errorlevel 1 (
    echo Error: GCC not found
    exit /b 1
)

REM Test MASM
echo Testing MASM...
dir "%VS_PATH%\ml64.exe" >nul
if errorlevel 1 (
    echo Error: MASM not found
    exit /b 1
)

REM Test Linker
echo Testing Linker...
dir "%VS_PATH%\link.exe" >nul
if errorlevel 1 (
    echo Error: Linker not found
    exit /b 1
)

echo All tools found!

REM Create build directories
if not exist obj mkdir obj
if not exist bin mkdir bin
if not exist obj\frontend mkdir obj\frontend
if not exist obj\frontend\lexer mkdir obj\frontend\lexer
if not exist obj\frontend\parser mkdir obj\frontend\parser
if not exist obj\middleend mkdir obj\middleend
if not exist obj\middleend\intermediate mkdir obj\middleend\intermediate
if not exist obj\middleend\optimization mkdir obj\middleend\optimization
if not exist obj\backend mkdir obj\backend
if not exist obj\backend\codegen mkdir obj\backend\codegen
if not exist obj\backend\registers mkdir obj\backend\registers
if not exist obj\backend\assembly mkdir obj\backend\assembly
if not exist obj\runtime mkdir obj\runtime

echo Directories created.

REM Build with make
echo Building with make...
make all

if errorlevel 1 (
    echo Build failed!
    exit /b 1
)

echo Build completed successfully!
echo Output: bin\schismc.exe

pause

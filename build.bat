@echo off
REM SchismC Build Script for Windows
REM True assembly-based HolyC compiler

echo Building SchismC...

REM Set up Microsoft Build Tools environment
set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64"
set "PATH=%VS_PATH%;%PATH%"

REM Check if gcc is available
gcc --version >nul 2>&1
if errorlevel 1 (
    echo Error: GCC not found. Please install MinGW-w64 or similar.
    echo Download from: https://www.mingw-w64.org/
    pause
    exit /b 1
)

REM Check if MASM is available
"%VS_PATH%\ml64.exe" /? >nul 2>&1
if errorlevel 1 (
    echo Error: MASM (ml64.exe) not found at %VS_PATH%
    echo Please verify Visual Studio installation.
    pause
    exit /b 1
)

REM Check if link.exe is available
"%VS_PATH%\link.exe" /? >nul 2>&1
if errorlevel 1 (
    echo Error: link.exe not found at %VS_PATH%
    echo Please verify Visual Studio installation.
    pause
    exit /b 1
)

echo All required tools found:
echo - GCC: Available
echo - MASM: Available at %VS_PATH%
echo - Link: Available at %VS_PATH%

REM Create build directories
if not exist obj mkdir obj
if not exist bin mkdir bin
if not exist obj\frontend\lexer mkdir obj\frontend\lexer
if not exist obj\frontend\parser mkdir obj\frontend\parser
if not exist obj\middleend\intermediate mkdir obj\middleend\intermediate
if not exist obj\middleend\optimization mkdir obj\middleend\optimization
if not exist obj\backend\codegen mkdir obj\backend\codegen
if not exist obj\backend\registers mkdir obj\backend\registers
if not exist obj\backend\assembly mkdir obj\backend\assembly
if not exist obj\runtime mkdir obj\runtime

REM Build using make
make all

if errorlevel 1 (
    echo Build failed!
    pause
    exit /b 1
)

echo Build successful!
echo Executable: bin\schismc.exe
pause

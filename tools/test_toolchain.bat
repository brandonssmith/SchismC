@echo off
REM Test Script for SchismC Toolchain
REM Verifies all required tools are available

echo Testing SchismC Toolchain...
echo =============================

REM Set up Microsoft Build Tools environment
set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64"

REM Test GCC
echo Testing GCC...
gcc --version >nul 2>&1
if errorlevel 1 (
    echo [FAIL] GCC not found
    goto :error
) else (
    echo [PASS] GCC available
)

REM Test MASM
echo Testing MASM...
"%VS_PATH%\ml64.exe" /? >nul 2>&1
if errorlevel 1 (
    echo [FAIL] MASM (ml64.exe) not found at %VS_PATH%
    goto :error
) else (
    echo [PASS] MASM available at %VS_PATH%
)

REM Test Linker
echo Testing Linker...
"%VS_PATH%\link.exe" /? >nul 2>&1
if errorlevel 1 (
    echo [FAIL] Linker (link.exe) not found at %VS_PATH%
    goto :error
) else (
    echo [PASS] Linker available at %VS_PATH%
)

REM Test Make
echo Testing Make...
make --version >nul 2>&1
if errorlevel 1 (
    echo [FAIL] Make not found
    goto :error
) else (
    echo [PASS] Make available
)

echo.
echo All tools are available! Toolchain is ready.
echo.
echo Next steps:
echo 1. Build the compiler: make all
echo 2. Test with: make test
echo 3. Run: bin\schismc.exe
goto :end

:error
echo.
echo [ERROR] Toolchain setup incomplete!
echo.
echo Required tools:
echo - GCC (MinGW-w64): https://www.mingw-w64.org/
echo - MASM: Install Visual Studio or Windows SDK
echo - Linker: Install Visual Studio or Windows SDK  
echo - Make: Install MinGW-w64 or MSYS2
echo.
echo Please install missing tools and run this script again.

:end
pause

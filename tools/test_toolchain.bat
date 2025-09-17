@echo off
echo Testing SchismC Toolchain...
echo =============================

echo Testing GCC...
gcc --version >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [FAIL] GCC not found
    goto :error
) else (
    echo [PASS] GCC available
)

echo Testing MASM...
set "MASM_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\ml64.exe"
if exist "%MASM_PATH%" (
    echo [PASS] MASM found at %MASM_PATH%
) else (
    echo [FAIL] MASM not found at %MASM_PATH%
    goto :error
)

echo Testing Linker...
set "LINK_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\link.exe"
if exist "%LINK_PATH%" (
    echo [PASS] Linker found at %LINK_PATH%
) else (
    echo [FAIL] Linker not found at %LINK_PATH%
    goto :error
)

echo Testing Make...
make --version >nul 2>&1
if %ERRORLEVEL% neq 0 (
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

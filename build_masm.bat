@echo off
REM Build script for SchismC using MASM + LINK
REM This script compiles HolyC source to MASM assembly, then assembles and links it

setlocal

REM Set MASM and LINK paths
set MASM_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx86\x64"
set LINK_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx86\x64"

REM Add MASM and LINK to PATH
set PATH=%MASM_PATH%;%LINK_PATH%;%PATH%

REM Check if input file is provided
if "%1"=="" (
    echo Usage: build_masm.bat ^<input.hc^> [output.exe]
    echo Example: build_masm.bat tests\hello_world.hc hello_world.exe
    exit /b 1
)

REM Set input and output files
set INPUT_FILE=%1
if "%2"=="" (
    set OUTPUT_FILE=%~n1.exe
) else (
    set OUTPUT_FILE=%2
)

set ASM_FILE=test_masm_output.asm
set OBJ_FILE=%~n1.obj

echo ========================================
echo SchismC MASM Build Script
echo ========================================
echo Input file: %INPUT_FILE%
echo Assembly file: %ASM_FILE%
echo Object file: %OBJ_FILE%
echo Output file: %OUTPUT_FILE%
echo ========================================

REM Step 1: Compile HolyC to MASM assembly
echo.
echo Step 1: Compiling HolyC to MASM assembly...
bin\schismc.exe %INPUT_FILE%
if errorlevel 1 (
    echo ERROR: Failed to compile HolyC to MASM assembly
    exit /b 1
)

REM Step 2: Assemble with MASM
echo.
echo Step 2: Assembling with MASM...
ml64 /c /Fo%OBJ_FILE% %ASM_FILE%
if errorlevel 1 (
    echo ERROR: Failed to assemble with MASM
    exit /b 1
)

REM Step 3: Link with LINK
echo.
echo Step 3: Linking with LINK...
link /SUBSYSTEM:CONSOLE /ENTRY:main /OUT:%OUTPUT_FILE% %OBJ_FILE% "C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\um\x64\kernel32.lib"
if errorlevel 1 (
    echo ERROR: Failed to link with LINK
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo Output: %OUTPUT_FILE%
echo ========================================

REM Test the executable
echo.
echo Testing the executable...
%OUTPUT_FILE%
if errorlevel 1 (
    echo WARNING: Executable returned error code %errorlevel%
) else (
    echo SUCCESS: Executable ran successfully!
)

REM Clean up intermediate files
echo.
echo Cleaning up intermediate files...
del %ASM_FILE% 2>nul
del %OBJ_FILE% 2>nul

echo Build process completed.

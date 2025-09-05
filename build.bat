@echo off
echo Building SchismC - A Windows Port of HolyC...

if not exist obj mkdir obj
if not exist obj\lexer mkdir obj\lexer
if not exist obj\parser mkdir obj\parser
if not exist obj\codegen mkdir obj\codegen
if not exist obj\runtime mkdir obj\runtime
if not exist bin mkdir bin

echo Compiling source files...

REM Try to find a C compiler
where cl >nul 2>&1
if %errorlevel% == 0 (
    echo Using Microsoft Visual C++ compiler...
    cl /nologo /Wall /Iinclude /c src\main.c /Foobj\main.obj
    cl /nologo /Wall /Iinclude /c src\lexer\lexer.c /Foobj\lexer\lexer.obj
    cl /nologo /Wall /Iinclude /c src\parser\parser.c /Foobj\parser\parser.obj
    cl /nologo /Wall /Iinclude /c src\codegen\codegen.c /Foobj\codegen\codegen.obj
    cl /nologo /Wall /Iinclude /c src\runtime\runtime.c /Foobj\runtime\runtime.obj
    cl /nologo obj\main.obj obj\lexer\lexer.obj obj\parser\parser.obj obj\codegen\codegen.obj obj\runtime\runtime.obj /Fe:bin\holyc.exe
) else (
    where gcc >nul 2>&1
    if %errorlevel% == 0 (
        echo Using GCC compiler...
        gcc -Wall -Wextra -std=c99 -O2 -g -Iinclude -c src\main.c -o obj\main.o
        gcc -Wall -Wextra -std=c99 -O2 -g -Iinclude -c src\lexer\lexer.c -o obj\lexer\lexer.o
        gcc -Wall -Wextra -std=c99 -O2 -g -Iinclude -c src\parser\parser.c -o obj\parser\parser.o
        gcc -Wall -Wextra -std=c99 -O2 -g -Iinclude -c src\codegen\codegen.c -o obj\codegen\codegen.o
        gcc -Wall -Wextra -std=c99 -O2 -g -Iinclude -c src\runtime\runtime.c -o obj\runtime\runtime.o
        gcc obj\main.o obj\lexer\lexer.o obj\parser\parser.o obj\codegen\codegen.o obj\runtime\runtime.o -o bin\schismc.exe
    ) else (
        echo Error: No C compiler found. Please install Visual Studio or MinGW.
        exit /b 1
    )
)

if %errorlevel% == 0 (
    echo Build successful! Executable created: bin\schismc.exe
) else (
    echo Build failed!
    exit /b 1
)

# SchismC

A true assembly-based Windows port of HolyC, the programming language from TempleOS.

## Overview

SchismC is a faithful reimplementation of HolyC that compiles directly to x86-64 assembly and generates native Windows executables. Unlike transpilers that generate C code, SchismC maintains the original HolyC philosophy of direct assembly integration and ahead-of-time compilation.

## Features

- **True Assembly-Based Compilation**: Generates x86-64 machine code directly
- **HolyC Language Compatibility**: Supports HolyC syntax and features
- **Function Support**: Complete function declarations, calls, and return statements
- **x64 Calling Convention**: Proper Windows x64 calling convention implementation
- **MASM Output Generation**: Generates Microsoft Macro Assembler (MASM) files
- **Inline Assembly Support**: Seamless mixing of HolyC and assembly code
- **Ahead-of-Time Compilation**: Generates native Windows PE executables
- **Optimization Passes**: Multiple optimization levels for performance
- **Windows Integration**: Native Windows API access and system calls

## Architecture

SchismC follows a traditional compiler architecture with three main phases:

### Frontend
- **Lexer**: Tokenizes HolyC source code
- **Parser**: Builds Abstract Syntax Tree (AST)

### Middle-End
- **Intermediate Code Generation**: Converts AST to intermediate representation
- **Optimization**: Multiple optimization passes for performance

### Backend
- **Assembly Generation**: Converts intermediate code to x86-64 assembly
- **MASM Output**: Generates Microsoft Macro Assembler files for linking
- **x64 Calling Convention**: Implements proper Windows x64 function calling
- **Register Allocation**: Efficient register management
- **AOT Compilation**: Generates native Windows executables

## Project Structure

```
SchismC/
├── src/
│   ├── frontend/
│   │   ├── lexer/          # Tokenization
│   │   └── parser/         # AST generation
│   ├── middleend/
│   │   ├── intermediate/   # IC generation
│   │   └── optimization/   # Optimization passes
│   ├── backend/
│   │   ├── aot/            # Ahead-of-time compilation
│   │   ├── assembly/       # Assembly generation and MASM output
│   │   ├── codegen/        # Code generation
│   │   └── registers/      # Register allocation
│   └── runtime/            # Runtime library
├── include/                # Header files
├── tests/                  # Test programs
│   ├── simple_console_demo.hc  # Function demonstration
│   ├── test_functions.hc       # Function test cases
│   └── *.hc                   # Various test programs
├── tools/                  # Build tools
│   ├── create_working_pe.c    # PE generation tool
│   └── test_toolchain.bat     # Toolchain testing
├── docs/                   # Documentation
├── bin/                    # Compiled executables
│   └── schismc.exe            # Main compiler executable
├── obj/                    # Object files
├── build_masm.bat          # MASM build script
├── run_demo.bat            # Console demo runner
├── demo_showcase.bat       # Feature demonstration
├── function_demo.asm       # Console application demo
└── output.asm              # Generated MASM output
```

## Building

### Prerequisites
- Windows 10/11
- GCC (MinGW-w64 recommended)
- Make utility
- Microsoft Macro Assembler (MASM) - for MASM output compilation
- Microsoft Linker (LINK) - for linking MASM object files

### Quick Build
```cmd
build.bat
```

### Manual Build
```cmd
make all
```

### Debug Build
```cmd
make debug
```

### Release Build
```cmd
make release
```

### MASM Build (Hybrid Approach)
```cmd
build_masm.bat
```
This builds using the hybrid approach: SchismC → MASM → LINK for generating working executables.

## Usage

### Basic Compilation
```cmd
schismc.exe input.hc -o output.exe
```

### MASM Output Generation
```cmd
schismc.exe input.hc
```
This generates `output.asm` (MASM assembly file) that can be assembled with MASM and linked with LINK.

### Console Application Demo
```cmd
run_demo.bat
```
Runs the console application demonstration showing function support in action.

## HolyC Language Features

### Built-in Types
- `I64`: 64-bit signed integer
- `F64`: 64-bit floating point
- `String`: String type
- `Bool`: Boolean type
- `U8`, `U16`, `U32`, `U64`: Unsigned integers
- `I8`, `I16`, `I32`: Signed integers

### Built-in Functions
- `Print()`: Output function with format specifiers
- `MAlloc()`: Memory allocation
- `Free()`: Memory deallocation
- `StrNew()`: String creation
- `StrPrint()`: String formatting

### Function Support
- **Function Declarations**: Full function signature parsing with parameters
- **Function Calls**: x64 calling convention with proper argument passing
- **Return Statements**: Support for simple values and complex expressions
- **Parameter Handling**: Proper parameter scope and type management
- **x64 Calling Convention**: RCX, RDX, R8, R9 for first 4 arguments, stack for additional

### Control Structures
- `if/else` statements
- `while` loops
- `for` loops
- `return` statements
- `break` and `continue`

### Assembly Integration
```holyc
I64 add_numbers(I64 a, I64 b) {
    I64 result;
    asm {
        mov rax, a
        add rax, b
        mov result, rax
    }
    return result;
}
```

## Example Programs

### Hello World
```holyc
"Hello, World!";
```

### Function Demo
```holyc
I64 get_hello_value() {
    return 42;
}

I64 get_world_value() {
    return 24;
}

I64 calculate_result() {
    return 66;
}
```

### Simple Calculator with Function Calls
```holyc
I64 add(I64 a, I64 b) {
    return a + b;
}

I64 multiply(I64 x, I64 y) {
    return x * y;
}

I64 main() {
    I64 a = 10;
    I64 b = 3;
    I64 sum = add(a, b);
    I64 product = multiply(a, b);
    return sum + product;
}
```

## MASM Output and Console Applications

SchismC can generate Microsoft Macro Assembler (MASM) files that can be assembled into working console applications. This hybrid approach provides a reliable way to create executable programs.

### Generated MASM Features

- **Function Prologues and Epilogues**: Proper x64 calling convention
- **Parameter Handling**: RCX, RDX, R8, R9 for first 4 arguments
- **Shadow Space**: 32-byte shadow space allocation and cleanup
- **Return Values**: Proper return value handling in RAX
- **External Functions**: Support for Windows API calls

### Example Generated MASM

```asm
; Generated by SchismC - MASM Assembly Output
; Target: Windows x64

.code

get_hello_value PROC
    ; Function prologue
    push rbp        ; Save caller's frame pointer
    mov rbp, rsp    ; Set up new frame pointer
    sub rsp, 32h    ; Allocate local space
    
    ; Function body
    mov rax, 42    ; Return value
    
    ; Function epilogue
    mov rsp, rbp    ; Restore stack pointer
    pop rbp         ; Restore caller's frame pointer
    ret             ; Return to caller
get_hello_value ENDP
```

### Console Application Demo

The project includes several demonstration programs:

- `tests/simple_console_demo.hc` - Basic function demonstration
- `function_demo.asm` - Complete console application
- `run_demo.bat` - Build and run script
- `demo_showcase.bat` - Feature demonstration

## Testing

Run the test suite:
```cmd
make test
```

Run function support tests:
```cmd
.\bin\schismc.exe tests\simple_console_demo.hc
```

## Development Status

This project is currently in active development. The following phases are planned:

- [x] Phase 1: Project structure and foundation
- [x] Phase 2: Frontend (Lexer + Parser)
- [x] Phase 3: Middle-end (Intermediate code + Optimization)
- [x] Phase 4: Backend (Assembly generation + MASM output)
- [x] Phase 5: Function Support (Declarations, calls, returns)
- [x] Phase 6: x64 Calling Convention Implementation
- [x] Phase 7: MASM Output Generation
- [x] Phase 8: Console Application Demo
- [ ] Phase 9: Variable support and expressions
- [ ] Phase 10: Control flow structures
- [ ] Phase 11: Assembly resolution and AOT compilation
- [ ] Phase 12: Testing and refinement

### Recently Completed Features

- ✅ **Function Declarations**: Complete parsing of function signatures with parameters
- ✅ **Function Calls**: x64 calling convention with proper argument passing
- ✅ **Return Statements**: Support for simple values and complex expressions
- ✅ **MASM Output**: Generates Microsoft Macro Assembler files
- ✅ **Console Applications**: Working console demos showing function support
- ✅ **x64 Calling Convention**: Proper Windows x64 function calling implementation

## Contributing

Contributions are welcome! Please see the development guidelines in the `docs/` directory.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- Terry A. Davis for creating TempleOS and HolyC
- The TempleOS community for preserving and documenting the original system
- The open source community for tools and inspiration

## References

- [TempleOS](https://templeos.org/)
- [HolyC Language Reference](https://templeos.org/HolyC.html)
- [x86-64 Assembly Reference](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)

>> "In the beginning was the Word, and the Word was with God, and the Word was God." - John 1:1

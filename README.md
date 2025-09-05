# SchismC - A Windows Port of HolyC

SchismC is a Windows port of the HolyC programming language from TempleOS. The name "SchismC" reflects the split from the original HolyC implementation, much like the historical Great Schism in the Catholic Church.

## Features

- **HolyC Language Support**: Implements core HolyC language features
- **Windows Native**: Built specifically for Windows with MinGW support
- **C Code Generation**: Compiles HolyC to standard C code
- **Automatic Print**: String literals automatically become print statements
- **Complete Toolchain**: Lexer, Parser, Code Generator, and Runtime

## Quick Start

### Compile a HolyC Program

```bash
# Compile HolyC to C
.\bin\schismc.exe examples\hello_world.hc

# Compile the generated C code
gcc examples\hello_world.c -o examples\hello_world.exe

# Run the program
.\examples\hello_world.exe
```

### Building the Compiler

```bash
# Using make
make

# Or using the batch script
.\build.bat
```

## Language Features

- String literals: `"Hello, World!";` → `Print("Hello, World!");`
- Built-in types: I0, I8, I16, I32, I64, U0, U8, U16, U32, U64, F64, Bool, String
- Function definitions and calls
- Control structures (if, while, for)
- Classes and unions
- Assembly integration

## Project Structure

```
SchismC/
├── bin/schismc.exe      # The compiler executable
├── src/                 # Source code
│   ├── main.c          # Main entry point
│   ├── lexer/          # Lexical analysis
│   ├── parser/         # Syntax analysis
│   ├── codegen/        # Code generation
│   └── runtime/        # Runtime library
├── include/            # Header files
├── examples/           # Example programs
├── tests/              # Test programs
├── Makefile           # Build system
└── build.bat          # Windows build script
```

## History

SchismC was created as a Windows port of the HolyC language from TempleOS. The original HolyC was designed by Terry A. Davis for the TempleOS operating system. SchismC brings this unique language to Windows while maintaining compatibility with core HolyC features.

## License

This project is open source and available under the MIT License.

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

---

*"In the beginning was the Word, and the Word was with God, and the Word was God." - John 1:1*

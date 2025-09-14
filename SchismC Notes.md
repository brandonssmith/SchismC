 SchismC Notes

Phase 1: Foundation & Project Structure

Goal: Set up the proper architecture and basic infrastructure
 1.Create Project Structure
 
 SchismC/
   ├── src/
   │   ├── frontend/
   │   │   ├── lexer/          # Tokenization
   │   │   └── parser/         # AST generation
   │   ├── middleend/
   │   │   ├── intermediate/   # IC generation
   │   │   └── optimization/   # Optimization passes
   │   ├── backend/
   │   │   ├── codegen/        # Assembly generation
   │   │   ├── registers/      # Register allocation
   │   │   └── assembly/       # Assembly resolution
   │   └── runtime/            # Runtime library
   ├── include/                # Headers
   ├── tools/                  # Build tools
   └── tests/                  # Test programs

2. Port Core Data Structures
- CCmpCtrl (Compiler Control structure)
    - Assembly-aware design: Added x86-64 specific state tracking
    - Register allocation: Built-in register management system
    - Memory layout: Stack frame and section size tracking
    - x86-64 features: Support for RIP-relative addressing, extended registers, SSE/AVX
- CIntermediateCode (Intermediate code representation)
    - Direct assembly mapping: Each IC operation maps to x86-64 instructions
    - Register allocation info: Tracks allocated registers and spill state
    - Memory layout awareness: Stack offsets and memory operand sizes
    - Assembly generation state: Tracks generated assembly bytes
- CAsmArg (Assembly argument handling)
    - Full x86-64 support: All register types (RAX, XMM0, etc.)
    - Memory addressing modes: Indirect, SIB, displacement, RIP-relative
    - Instruction encoding: REX prefix, ModR/M, SIB byte support
    - Assembly-specific flags: Immediate, register, memory, absolute addressing
- CAOT (Ahead-of-Time compilation structures)
    - Native code generation: Direct binary code output
    - Import/Export tables: Symbol resolution for PE format
    - Heap management: Global variable allocation
    - Binary block management: Efficient code generation
  
  # Key Assembly-Centric Features
    
    * Register Management
    > X86Register allocate_register(CCmpCtrl *cc, I64 size);
    > void free_register(CCmpCtrl *cc, X86Register reg);
    > Bool is_register_allocated(CCmpCtrl *cc, X86Register reg);

  # Assembly Generation
    > U8* generate_assembly_instruction(CIntermediateCode *ic, I64 *size);
    > Bool encode_x86_instruction(CAsmArg *arg1, CAsmArg *arg2, U8 opcode, U8 *output, I64 *size);

  # Memory LAyout
    > I64 allocate_stack_space(CCmpCtrl *cc, I64 size);
    > I64 allocate_global_data(CCmpCtrl *cc, I64 size);
    > void set_memory_alignment(CCmpCtrl *cc, I64 alignment);

Phase 2: Frontend (Lexer + Parser)
Goal: Parse HolyC source into AST
1. Enhanced Lexer
- Port token types and keyword recognition
- Handle HolyC-specific syntax (range comparisons, etc.)
- Assembly token recognition

2. Parser
- Statement parsing (PrsStmt.HC)
- Expression parsing (PrsExp.HC)
- Variable parsing (PrsVar.HC)
- Assembly parsing (PrsAsm* functions)


Phase 3: Middle-End (Intermediate Code)
Goal: Generate optimized intermediate code
1. Intermediate Code Generation
- Port LexStmt2Bin() functionality
- Generate instruction codes (ICs)
- Handle register allocation hints

2. Optimization Passes
- Port optimization passes 0-9 (OptPass*.HC)
- Dead code elimination
- Constant folding
- Register optimization

Phase 3: Notes
Assembly-Centric Intermediate Code Features Implemented:
1. LexStmt2Bin Equivalent
- ic_gen_compile_statement() - Main compilation function
- Statement-to-binary compilation pipeline
- Assembly-aware intermediate code generation

2. HolyC Optimization Passes (0-9)
- Pass 0-2: Constant folding and type determination
- Pass 3: Register allocation optimization (x86-64 aware)
- Pass 4: Memory layout optimization
- Pass 5: Dead code elimination
- Pass 6: Control flow optimization
- Pass 7-9: Assembly generation and final optimization

3. Assembly-Centric Features
- Register Allocation: x86-64 register management (RAX-R15, XMM0-XMM15)
- Instruction Encoding: REX prefix, ModR/M, SIB byte generation
- Memory Layout: Stack offset calculation and alignment
- Assembly Generation: Direct x86-64 instruction generation

4. Intermediate Code Operations
- Arithmetic: ADD, SUB, MUL, DIV, MOD with assembly mapping
- Assembly-Specific: IC_ASM_INLINE, IC_ASM_REG_ALLOC, IC_ASM_MEM_ACCESS
- HolyC-Specific: IC_DOT_DOT (range), IC_DOLLAR_EXPR, IC_TRY_CATCH
- AOT Support: IC_AOT_STORE, IC_AOT_RESOLVE, IC_AOT_PATCH

5. Optimization Features
- Constant Folding: Arithmetic expression simplification
- Dead Code Elimination: Unused instruction removal
- Register Optimization: x86-64 register allocation
- Memory Access Optimization: Stack layout optimization
- Key Assembly-Centric Design Decisions:
- Direct Assembly Mapping: Intermediate code directly maps to x86-64 instructions
- Register-Aware: All optimizations consider x86-64 register constraints
- Memory-Centric: Stack and heap layout optimized for assembly generation
- AOT-Ready: Ahead-of-time compilation support built-in


Phase 4: Backend (Assembly Generation)
Goal: Generate x86-64 assembly code
1. Assembly Code Generation
- Port BackA.HC, BackB.HC, BackC.HC
- Generate x86-64 machine code bytes
- Handle instruction encoding

2. Register Management
- Port register allocation algorithms
- Handle RAX/RBX/RCX/RDX register usage
- Implement register spilling

# Phase 4 notes:

1. Machine Code Generation (HolyC Equivalent)
- mc_emit_u8/u16/u24/u32/u64() - Direct machine code byte generation
- Equivalent to HolyC's ICU8, ICU16, ICU24, ICU32 functions
- Raw x86-64 instruction encoding

2. x86-64 Instruction Encoding
- REX Prefix: Extended register support (R8-R15, XMM0-XMM15)
- ModR/M Byte: Register and memory addressing modes
- SIB Byte: Scale-Index-Base addressing for complex memory access
- Displacement: 8-bit, 16-bit, 32-bit, and 64-bit displacement values
- Immediate: Immediate operand encoding

3. Register Management System
- Register Allocation: RAX, RCX, RDX, RBX, RSI, RDI, R8-R11
- Register Tracking: Usage monitoring and allocation
- Register Spilling: Stack-based register spilling when needed
- Extended Registers: Full R8-R15 support with REX prefix

4. Assembly Instruction Generation
- Arithmetic: MOV, ADD, SUB, MUL, DIV, MOD with full x86-64 encoding
- Logical: AND, OR, XOR, NOT operations
- Shift: SHL, SHR operations
- Comparison: CMP, TEST operations
- Control Flow: JMP, conditional jumps, CALL, RET
- Stack: PUSH, POP, ENTER, LEAVE
- Memory: LOAD, STORE, LEA operations

5. Assembly Argument Handling
- Register Arguments: Direct register operands
- Immediate Arguments: Immediate values (8/16/32/64-bit)
- Memory Arguments: Complex addressing modes with displacement
- Absolute Arguments: Absolute memory addresses

6. Assembly Context Management
- Buffer Management: Dynamic assembly buffer allocation
- Instruction Pointer: Track current assembly position
- Stack Management: Stack frame and offset tracking
- Register State: Complete register allocation state

# Key Assembly-Centric Design Decisions:

- Direct Machine Code: Generates actual x86-64 machine code bytes
- Register-Aware: All instructions consider x86-64 register constraints
- Memory-Centric: Complex addressing modes for efficient memory access
- REX-Aware: Proper handling of extended registers and 64-bit mode
- HolyC-Compatible: Maintains compatibility with HolyC's assembly philosophy

# Assembly Generation Pipeline:

1. Intermediate Code → Assembly Context → Machine Code Bytes
2. Register Allocation → Instruction Encoding → Buffer Output
3. REX Prefix → Opcode → ModR/M → SIB → Displacement → Immediate


Phase 5: Assembly Resolution & AOT
Goal: Resolve symbols and generate native executables
1. Assembly Resolution
- Port AsmResolve.HC
- Handle assembly symbol resolution
- Generate relocation information

2. AOT Compilation
- Port CmpJoin() functionality
- Generate PE executable format
- Handle imports/exports

# Phase 5 Notes
Assembly Resolution & AOT Features Implemented:
1. Assembly Symbol Resolution (HolyC AsmResolve.HC equivalent)
- Import Resolution: Windows API and external symbol resolution
- Export Resolution: Symbol export for linking
- Relocation Resolution: Address fixup for jumps and calls
- Symbol Management: Hash table-based symbol lookup

2. AOT Compilation (HolyC CmpJoin() equivalent)
- CmpJoin Functionality: Complete AOT compilation pipeline
- Binary Generation: Native machine code output
- Memory Layout: Stack, heap, and code section management
- Symbol Resolution: Complete symbol resolution pipeline

3. PE Executable Format Generation
- PE COFF Header: x86-64 machine type, section count, characteristics
- PE Optional Header: Entry point, image base, stack/heap sizes
- Section Headers: .text (code), .data (initialized), .rdata (read-only)
- Import/Export Tables: Windows API integration ready

4. Native Executable Generation
- Binary Output: Direct PE executable file generation
- Memory Alignment: Proper section and file alignment
 -Checksum Calculation: PE file integrity validation
- Windows Integration: Console subsystem, proper entry points

5. Assembly-Centric AOT Features
- Direct Machine Code: Generates actual x86-64 machine code
- Register Allocation: x86-64 register management in AOT
- Memory Addressing: Complex addressing modes in native code
- Instruction Encoding: Full x86-64 instruction encoding

Key AOT Design Decisions:
- True Native Compilation: Generates actual Windows PE executables
- Assembly-Centric: All AOT features maintain assembly philosophy
- PE Format Compliant: Full Windows PE format support
- HolyC Compatible: Maintains compatibility with HolyC's AOT approach

AOT Compilation Pipeline:
- Source Code → Lexer → Parser → Intermediate Code
- Intermediate Code → Optimization Passes → Assembly Generation
- Assembly Code → Symbol Resolution → PE Generation
- PE Headers → Binary Output → Native Executable

PE Format Features:
- Machine Type: x86-64 (AMD64)
- Subsystem: Console application
- Sections: .text (code), .data (data), .rdata (constants)
- Memory Layout: 64-bit addressing, proper alignment
- Entry Point: Standard Windows entry point

Phase 6: Testing & Refinement
Goal: Ensure correctness and performance


CURRENT TODO:

- Add More Parser Features - Support for:

  - functions,
  - control flow, 
  - etc.


Test with Complex Programs - Multi-statement programs, expressions, etc.

Move on to function calls next?
Implement function assembly generation?

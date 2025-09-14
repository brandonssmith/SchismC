/*
 * SchismC Code Generator Header
 * x86-64 assembly generation
 */

#ifndef CODEGEN_H
#define CODEGEN_H

#include "holyc.h"
#include "intermediate.h"

/* x86-64 Register definitions */
typedef enum {
    REG_RAX = 0,
    REG_RBX,
    REG_RCX,
    REG_RDX,
    REG_RSI,
    REG_RDI,
    REG_RSP,
    REG_RBP,
    REG_R8,
    REG_R9,
    REG_R10,
    REG_R11,
    REG_R12,
    REG_R13,
    REG_R14,
    REG_R15,
    REG_COUNT,
    REG_NONE = -1
} Register;

/* Assembly instruction types */
typedef enum {
    ASM_MOV,
    ASM_ADD,
    ASM_SUB,
    ASM_MUL,
    ASM_DIV,
    ASM_MOD,
    ASM_CMP,
    ASM_JE,
    ASM_JNE,
    ASM_JL,
    ASM_JG,
    ASM_JLE,
    ASM_JGE,
    ASM_JMP,
    ASM_CALL,
    ASM_RET,
    ASM_PUSH,
    ASM_POP,
    ASM_LEA,
    ASM_SYSCALL,
    ASM_COUNT
} AsmInstruction;

/* Assembly operand types */
typedef enum {
    ASM_OP_REG,                  /* Register */
    ASM_OP_IMM,                  /* Immediate value */
    ASM_OP_MEM,                  /* Memory address */
    ASM_OP_LABEL                 /* Label reference */
} AsmOperandType;

/* Assembly operand */
typedef struct {
    AsmOperandType type;
    Register reg;                /* Register (if type is REG) */
    I64 immediate;               /* Immediate value (if type is IMM) */
    String label;                /* Label name (if type is LABEL) */
    Register base_reg;           /* Base register for memory (if type is MEM) */
    I64 offset;                  /* Offset for memory (if type is MEM) */
} AsmOperand;

/* Assembly instruction */
typedef struct {
    AsmInstruction type;
    AsmOperand operand1;
    AsmOperand operand2;
    String label;                /* Label for this instruction */
    I64 size;                    /* Instruction size in bytes */
    U8* bytes;                   /* Generated machine code */
} AsmInstruction;

/* Code generator state */
typedef struct {
    U8* code;                    /* Generated machine code */
    I64 code_size;               /* Size of generated code */
    I64 code_capacity;           /* Current capacity */
    I64 current_offset;          /* Current code offset */
    Register* register_usage;    /* Register usage tracking */
    I64* variable_offsets;       /* Stack frame variable offsets */
    I64 stack_offset;            /* Current stack offset */
    I64 label_count;             /* Number of labels */
    String* labels;              /* Label names */
    I64* label_addresses;        /* Label addresses */
} CodeGenState;

/* Function prototypes */
extern void init_codegen(CodeGenState* cg);
extern void cleanup_codegen(CodeGenState* cg);
extern void generate_assembly(CodeGenState* cg, ICGenerator* ic);
extern void generate_instruction(CodeGenState* cg, ICInstruction* ic);
extern void emit_instruction(CodeGenState* cg, AsmInstruction* asm_instr);
extern void emit_byte(CodeGenState* cg, U8 byte);
extern void emit_bytes(CodeGenState* cg, U8* bytes, I64 count);
extern void emit_immediate(CodeGenState* cg, I64 value, I64 size);
extern Register allocate_register(CodeGenState* cg);
extern void free_register(CodeGenState* cg, Register reg);
extern I64 get_variable_offset(CodeGenState* cg, String name);
extern I64 allocate_stack_space(CodeGenState* cg, I64 size);
extern I64 create_label(CodeGenState* cg, String name);
extern void resolve_labels(CodeGenState* cg);
extern void print_assembly(CodeGenState* cg, FILE* output);

/* x86-64 specific functions */
extern U8* encode_mov_reg_imm(Register reg, I64 immediate, I64* size);
extern U8* encode_add_reg_reg(Register dst, Register src, I64* size);
extern U8* encode_sub_reg_reg(Register dst, Register src, I64* size);
extern U8* encode_mul_reg_reg(Register dst, Register src, I64* size);
extern U8* encode_div_reg_reg(Register dst, Register src, I64* size);
extern U8* encode_cmp_reg_reg(Register reg1, Register reg2, I64* size);
extern U8* encode_jump_conditional(I64 condition, I64 offset, I64* size);
extern U8* encode_call_relative(I64 offset, I64* size);
extern U8* encode_ret(I64* size);
extern U8* encode_push_reg(Register reg, I64* size);
extern U8* encode_pop_reg(Register reg, I64* size);
extern U8* encode_syscall(I64* size);

#endif /* CODEGEN_H */

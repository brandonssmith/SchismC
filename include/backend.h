/*
 * Assembly Backend Header
 * x86-64 assembly generation for SchismC
 * Based on HolyC's BackA.HC, BackB.HC, BackC.HC functionality
 */

#ifndef BACKEND_H
#define BACKEND_H

#include "core_structures.h"
#include "intermediate.h"
#include "parser.h"

/* Assembly Generation Context */
typedef struct {
    CCmpCtrl *cc;                    /* Compiler control */
    ICGenContext *ic_ctx;            /* Intermediate code context */
    ParserState *parser;             /* Parser state for symbol resolution */

    /* Assembly generation state */
    U8 *assembly_buffer;             /* Generated assembly buffer */
    I64 buffer_size;                 /* Current buffer size */
    I64 buffer_capacity;             /* Buffer capacity */
    I64 instruction_pointer;         /* Current instruction pointer */
    
    /* Register management */
    X86Register allocated_regs[MAX_X86_REGS];  /* Allocated registers */
    Bool reg_in_use[MAX_X86_REGS];   /* Register usage tracking */
    I64 reg_count;                   /* Number of allocated registers */
    
    /* Stack management */
    I64 stack_offset;                /* Current stack offset */
    I64 max_stack_depth;             /* Maximum stack depth used */
    Bool stack_frame_created;        /* Whether stack frame is created */
    
    /* Assembly-specific flags */
    Bool use_64bit_mode;             /* 64-bit mode enabled */
    Bool use_rex_prefix;             /* REX prefix needed */
    Bool use_sib_addressing;         /* SIB addressing needed */
    Bool use_rip_relative;           /* RIP-relative addressing */
} AssemblyContext;

/* Machine Code Generation Functions (HolyC equivalent) */
typedef struct {
    U8 *buffer;                      /* Output buffer */
    I64 offset;                      /* Current offset */
    I64 capacity;                    /* Buffer capacity */
} MachineCodeBuffer;

/* Function Prototypes */

/* Core Assembly Generation */
AssemblyContext* assembly_context_new(CCmpCtrl *cc, ICGenContext *ic_ctx, ParserState *parser);
void assembly_context_free(AssemblyContext *ctx);
U8* assembly_generate_code(AssemblyContext *ctx, I64 *size);

/* Machine Code Generation (HolyC ICU8/ICU16/ICU24/ICU32 equivalent) */
Bool mc_emit_u8(MachineCodeBuffer *buf, U8 value);
Bool mc_emit_u16(MachineCodeBuffer *buf, U16 value);
Bool mc_emit_u24(MachineCodeBuffer *buf, U32 value);
Bool mc_emit_u32(MachineCodeBuffer *buf, U32 value);
Bool mc_emit_u64(MachineCodeBuffer *buf, U64 value);

/* x86-64 Instruction Encoding */
Bool asm_emit_rex_prefix(AssemblyContext *ctx, U8 rex);
Bool asm_emit_opcode(AssemblyContext *ctx, U8 opcode);
Bool asm_emit_modrm(AssemblyContext *ctx, U8 mod, U8 reg, U8 rm);
Bool asm_emit_sib(AssemblyContext *ctx, U8 scale, U8 index, U8 base);
Bool asm_emit_displacement(AssemblyContext *ctx, I64 disp, I64 size);
Bool asm_emit_immediate(AssemblyContext *ctx, I64 imm, I64 size);

/* Register Management */
X86Register asm_allocate_register(AssemblyContext *ctx, I64 size);
void asm_free_register(AssemblyContext *ctx, X86Register reg);
Bool asm_is_register_allocated(AssemblyContext *ctx, X86Register reg);
X86Register asm_spill_register(AssemblyContext *ctx, X86Register reg);

/* Assembly Instruction Generation */
Bool asm_generate_mov(AssemblyContext *ctx, CAsmArg *dst, CAsmArg *src);
Bool asm_generate_add(AssemblyContext *ctx, CAsmArg *dst, CAsmArg *src);
Bool asm_generate_sub(AssemblyContext *ctx, CAsmArg *dst, CAsmArg *src);
Bool asm_generate_mul(AssemblyContext *ctx, CAsmArg *dst, CAsmArg *src);
Bool asm_generate_div(AssemblyContext *ctx, CAsmArg *dst, CAsmArg *src);
Bool asm_generate_mod(AssemblyContext *ctx, CAsmArg *dst, CAsmArg *src);

/* Relational and Logical Operation Generation */
Bool asm_generate_cmp_eq(AssemblyContext *ctx, CAsmArg *result, CAsmArg *left, CAsmArg *right);
Bool asm_generate_cmp_ne(AssemblyContext *ctx, CAsmArg *result, CAsmArg *left, CAsmArg *right);
Bool asm_generate_cmp_lt(AssemblyContext *ctx, CAsmArg *result, CAsmArg *left, CAsmArg *right);
Bool asm_generate_cmp_le(AssemblyContext *ctx, CAsmArg *result, CAsmArg *left, CAsmArg *right);
Bool asm_generate_cmp_gt(AssemblyContext *ctx, CAsmArg *result, CAsmArg *left, CAsmArg *right);
Bool asm_generate_cmp_ge(AssemblyContext *ctx, CAsmArg *result, CAsmArg *left, CAsmArg *right);
Bool asm_generate_logical_and(AssemblyContext *ctx, CAsmArg *result, CAsmArg *left, CAsmArg *right);
Bool asm_generate_logical_or(AssemblyContext *ctx, CAsmArg *result, CAsmArg *left, CAsmArg *right);

/* Unary Operation Generation */
Bool asm_generate_logical_not(AssemblyContext *ctx);
Bool asm_generate_unary_minus(AssemblyContext *ctx);
Bool asm_generate_bitwise_not(AssemblyContext *ctx);
Bool asm_generate_pre_increment(AssemblyContext *ctx);
Bool asm_generate_pre_decrement(AssemblyContext *ctx);
Bool asm_generate_address_of(AssemblyContext *ctx);
Bool asm_generate_dereference(AssemblyContext *ctx);

/* Logical Operations */
Bool asm_generate_and(AssemblyContext *ctx, CAsmArg *dst, CAsmArg *src);
Bool asm_generate_or(AssemblyContext *ctx, CAsmArg *dst, CAsmArg *src);
Bool asm_generate_xor(AssemblyContext *ctx, CAsmArg *dst, CAsmArg *src);
Bool asm_generate_not(AssemblyContext *ctx, CAsmArg *dst);

/* Shift Operations */
Bool asm_generate_shl(AssemblyContext *ctx, CAsmArg *dst, CAsmArg *src);
Bool asm_generate_shr(AssemblyContext *ctx, CAsmArg *dst, CAsmArg *src);

/* Comparison Operations */
Bool asm_generate_cmp(AssemblyContext *ctx, CAsmArg *dst, CAsmArg *src);
Bool asm_generate_test(AssemblyContext *ctx, CAsmArg *dst, CAsmArg *src);

/* Control Flow */
Bool asm_generate_jmp(AssemblyContext *ctx, I64 target);
Bool asm_generate_jmp_conditional(AssemblyContext *ctx, U8 condition, I64 target);
Bool asm_generate_call(AssemblyContext *ctx, I64 target);
Bool asm_generate_ret(AssemblyContext *ctx);

/* Stack Operations */
Bool asm_generate_push(AssemblyContext *ctx, CAsmArg *src);
Bool asm_generate_pop(AssemblyContext *ctx, CAsmArg *dst);
Bool asm_generate_enter(AssemblyContext *ctx, I64 frame_size);
Bool asm_generate_leave(AssemblyContext *ctx);

/* Memory Operations */
Bool asm_generate_load(AssemblyContext *ctx, CAsmArg *dst, CAsmArg *src);
Bool asm_generate_store(AssemblyContext *ctx, CAsmArg *dst, CAsmArg *src);
Bool asm_generate_lea(AssemblyContext *ctx, CAsmArg *dst, CAsmArg *src);

/* HolyC-Specific Assembly Generation */
Bool asm_generate_print(AssemblyContext *ctx, CAsmArg *format, CAsmArg *args);
Bool asm_generate_malloc(AssemblyContext *ctx, CAsmArg *dst, CAsmArg *size);
Bool asm_generate_free(AssemblyContext *ctx, CAsmArg *ptr);

/* Inline Assembly Support */
Bool asm_generate_inline_assembly(AssemblyContext *ctx, U8 *assembly_code, I64 size);

/* Assembly Argument Handling */
Bool asm_setup_register_arg(AssemblyContext *ctx, CAsmArg *arg, X86Register reg);
Bool asm_setup_immediate_arg(AssemblyContext *ctx, CAsmArg *arg, I64 value);
Bool asm_setup_memory_arg(AssemblyContext *ctx, CAsmArg *arg, X86Register base, I64 offset);
Bool asm_setup_absolute_arg(AssemblyContext *ctx, CAsmArg *arg, I64 address);

/* Utility Functions */
Bool asm_needs_rex_prefix(CAsmArg *arg1, CAsmArg *arg2);
U8 asm_calculate_rex_prefix(CAsmArg *arg1, CAsmArg *arg2);
U8 asm_calculate_modrm_byte(CAsmArg *arg1, CAsmArg *arg2);
Bool asm_needs_sib_addressing(CAsmArg *arg);
I64 asm_calculate_instruction_size(CAsmArg *arg1, CAsmArg *arg2, U8 opcode);

/* Assembly Context Management */
Bool asm_expand_buffer(AssemblyContext *ctx, I64 additional_size);
Bool asm_reserve_space(AssemblyContext *ctx, I64 size);

/* Debug and Analysis */
void asm_print_assembly(AssemblyContext *ctx);
void asm_print_registers(AssemblyContext *ctx);
I64 asm_get_instruction_count(AssemblyContext *ctx);

/* AST-to-Assembly Direct Conversion */
Bool ast_to_assembly_generate(AssemblyContext *ctx, ASTNode *ast);
Bool ast_to_assembly_node(AssemblyContext *ctx, ASTNode *node);
Bool ast_to_assembly_string_literal(AssemblyContext *ctx, ASTNode *node);
Bool ast_to_assembly_integer_literal(AssemblyContext *ctx, ASTNode *node);
Bool ast_to_assembly_float_literal(AssemblyContext *ctx, ASTNode *node);
Bool ast_to_assembly_char_literal(AssemblyContext *ctx, ASTNode *node);
Bool ast_to_assembly_binary_operation(AssemblyContext *ctx, ASTNode *node);
Bool ast_to_assembly_unary_operation(AssemblyContext *ctx, ASTNode *node);
Bool ast_to_assembly_function_call(AssemblyContext *ctx, ASTNode *node);
Bool ast_to_assembly_assignment(AssemblyContext *ctx, ASTNode *node);
Bool ast_to_assembly_variable_declaration(AssemblyContext *ctx, ASTNode *node);
Bool ast_to_assembly_variable_reference(AssemblyContext *ctx, ASTNode *node);
Bool ast_to_assembly_function_declaration(AssemblyContext *ctx, ASTNode *node);
Bool ast_to_assembly_if_statement(AssemblyContext *ctx, ASTNode *node);
Bool ast_to_assembly_while_statement(AssemblyContext *ctx, ASTNode *node);
Bool ast_to_assembly_for_statement(AssemblyContext *ctx, ASTNode *node);
Bool ast_to_assembly_return_statement(AssemblyContext *ctx, ASTNode *node);
Bool ast_to_assembly_block_statement(AssemblyContext *ctx, ASTNode *node);
Bool ast_to_assembly_inline_assembly(AssemblyContext *ctx, ASTNode *node);

#endif /* BACKEND_H */

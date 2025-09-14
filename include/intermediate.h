/*
 * Intermediate Code Generation Header
 * Assembly-centric intermediate code generation for SchismC
 * Based on HolyC's LexStmt2Bin functionality
 */

#ifndef INTERMEDIATE_H
#define INTERMEDIATE_H

#include "core_structures.h"
#include "lexer.h"
#include "parser.h"

/* Intermediate Code Operation Types */
typedef enum {
    IC_NOP = 0,
    IC_ADD, IC_SUB, IC_MUL, IC_DIV, IC_MOD,
    IC_AND, IC_OR, IC_XOR, IC_NOT,
    IC_SHL, IC_SHR,
    IC_EQU, IC_NOT_EQU, IC_LESS, IC_GREATER, IC_LESS_EQU, IC_GREATER_EQU,
    IC_ASSIGN, IC_ADD_ASSIGN, IC_SUB_ASSIGN, IC_MUL_ASSIGN, IC_DIV_ASSIGN,
    IC_CALL, IC_RETURN, IC_RETURN_VAL,
    IC_JUMP, IC_JUMP_TRUE, IC_JUMP_FALSE,
    IC_PUSH, IC_POP,
    IC_LOAD, IC_STORE,
    IC_CAST,
    IC_PRINT, IC_PRINTF,
    IC_MALLOC, IC_FREE,
    
    /* Assembly-specific operations */
    IC_ASM_INLINE,      /* Inline assembly block */
    IC_ASM_REG_ALLOC,   /* Register allocation */
    IC_ASM_MEM_ACCESS,  /* Memory access with addressing modes */
    IC_ASM_IMMEDIATE,   /* Immediate value */
    IC_ASM_JUMP_TABLE,  /* Jump table for switch statements */
    
    /* HolyC-specific operations */
    IC_DOT_DOT,         /* Range operator .. */
    IC_DOLLAR_EXPR,     /* Dollar expression $ */
    IC_CLASS_ACCESS,    /* Class member access */
    IC_TRY_CATCH,       /* Exception handling */
    IC_THROW,           /* Exception throwing */
    
    /* AOT compilation operations */
    IC_AOT_STORE,       /* AOT code storage */
    IC_AOT_RESOLVE,     /* AOT symbol resolution */
    IC_AOT_PATCH        /* AOT code patching */
} ICOperation;

/* Intermediate Code Generation Context */
typedef struct {
    CCmpCtrl *cc;                    /* Compiler control */
    CIntermediateCode *ic_head;      /* Head of IC chain */
    CIntermediateCode *ic_tail;      /* Tail of IC chain */
    I64 ic_count;                    /* Number of IC instructions */
    
    /* Assembly-specific state */
    X86Register allocated_regs[MAX_X86_REGS];  /* Allocated registers */
    I64 reg_count;                   /* Number of allocated registers */
    I64 stack_offset;                /* Current stack offset */
    I64 instruction_pointer;         /* Current instruction pointer */
    
    /* Optimization state */
    Bool optimization_enabled;       /* Whether optimizations are enabled */
    I64 optimization_level;          /* Optimization level (0-9) */
    Bool dead_code_elimination;      /* Dead code elimination enabled */
    Bool constant_folding;           /* Constant folding enabled */
    Bool register_optimization;      /* Register optimization enabled */
} ICGenContext;

/* Optimization Pass Functions */
typedef struct {
    I64 pass_number;                 /* Pass number (0-9) */
    char *pass_name;                 /* Human-readable pass name */
    Bool (*pass_function)(ICGenContext *ctx);  /* Pass function pointer */
    Bool enabled;                    /* Whether pass is enabled */
} OptimizationPass;

/* Function Prototypes */

/* Core Intermediate Code Generation */
ICGenContext* ic_gen_context_new(CCmpCtrl *cc);
void ic_gen_context_free(ICGenContext *ctx);
CIntermediateCode* ic_gen_add_instruction(ICGenContext *ctx, ICOperation op, CAsmArg *arg1, CAsmArg *arg2, CAsmArg *result);
CIntermediateCode* ic_gen_add_assembly(ICGenContext *ctx, U8 opcode, CAsmArg *arg1, CAsmArg *arg2);

/* LexStmt2Bin equivalent - main compilation function */
U8* ic_gen_compile_statement(ICGenContext *ctx, I64 *type, I64 cmp_flags);

/* Optimization Passes (HolyC OptPass0-9 equivalent) */
Bool opt_pass_012(ICGenContext *ctx);  /* Constant folding and type determination */
Bool opt_pass_3(ICGenContext *ctx);    /* Register allocation optimization */
Bool opt_pass_4(ICGenContext *ctx);    /* Memory layout optimization */
Bool opt_pass_5(ICGenContext *ctx);    /* Dead code elimination */
Bool opt_pass_6(ICGenContext *ctx);    /* Control flow optimization */
Bool opt_pass_789(ICGenContext *ctx);  /* Assembly generation and final optimization */

/* Assembly-specific optimizations */
Bool opt_register_allocation(ICGenContext *ctx);
Bool opt_instruction_scheduling(ICGenContext *ctx);
Bool opt_memory_access_optimization(ICGenContext *ctx);

/* Constant folding and propagation */
Bool opt_constant_folding(ICGenContext *ctx);
Bool opt_constant_propagation(ICGenContext *ctx);

/* Dead code elimination */
Bool opt_dead_code_elimination(ICGenContext *ctx);
Bool opt_unreachable_code_elimination(ICGenContext *ctx);

/* Control flow optimization */
Bool opt_branch_optimization(ICGenContext *ctx);
Bool opt_loop_optimization(ICGenContext *ctx);

/* Utility functions */
CIntermediateCode* ic_find_next_use(CIntermediateCode *start, X86Register reg);
Bool ic_is_dead(CIntermediateCode *ic);
I64 ic_calculate_cost(CIntermediateCode *ic);

/* Assembly generation from intermediate code */
U8* ic_generate_assembly(ICGenContext *ctx, I64 *size);
Bool ic_emit_instruction(ICGenContext *ctx, CIntermediateCode *ic, U8 *output, I64 *offset);

/* AOT compilation support */
Bool ic_prepare_aot(ICGenContext *ctx, CAOT *aot);
Bool ic_resolve_symbols(ICGenContext *ctx, CAOT *aot);

/* AST-to-Intermediate Code Conversion */
Bool ic_gen_from_ast(ICGenContext *ctx, ASTNode *ast);
Bool ic_gen_ast_node(ICGenContext *ctx, ASTNode *node);

/* Expression Evaluation and Constant Folding */
Bool ic_is_constant_expression(ASTNode *node);
ASTNode* ic_fold_constant_expression(ASTNode *node);
ASTNode* ic_evaluate_binary_operation(ASTNode *left, ASTNode *right, BinaryOpType op);
ASTNode* ic_evaluate_unary_operation(ASTNode *operand, UnaryOpType op);
ASTNode* ic_copy_ast_node(ASTNode *node);
const char* ic_get_operator_string(BinaryOpType op);
const char* ic_get_unary_operator_string(UnaryOpType op);
Bool ic_gen_string_literal(ICGenContext *ctx, ASTNode *node);
Bool ic_gen_integer_literal(ICGenContext *ctx, ASTNode *node);
Bool ic_gen_float_literal(ICGenContext *ctx, ASTNode *node);
Bool ic_gen_char_literal(ICGenContext *ctx, ASTNode *node);
Bool ic_gen_identifier(ICGenContext *ctx, ASTNode *node);
Bool ic_gen_binary_operation(ICGenContext *ctx, ASTNode *node);
Bool ic_gen_unary_operation(ICGenContext *ctx, ASTNode *node);
Bool ic_gen_function_call(ICGenContext *ctx, ASTNode *node);
Bool ic_gen_assignment(ICGenContext *ctx, ASTNode *node);
Bool ic_gen_variable_declaration(ICGenContext *ctx, ASTNode *node);
Bool ic_gen_function_declaration(ICGenContext *ctx, ASTNode *node);
Bool ic_gen_if_statement(ICGenContext *ctx, ASTNode *node);
Bool ic_gen_while_statement(ICGenContext *ctx, ASTNode *node);
Bool ic_gen_for_statement(ICGenContext *ctx, ASTNode *node);
Bool ic_gen_return_statement(ICGenContext *ctx, ASTNode *node);
Bool ic_gen_block_statement(ICGenContext *ctx, ASTNode *node);
Bool ic_gen_assembly_block(ICGenContext *ctx, ASTNode *node);

#endif /* INTERMEDIATE_H */
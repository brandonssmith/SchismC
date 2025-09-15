/*
 * MASM Assembly Output Header
 * Microsoft Macro Assembler (MASM) assembly code generation for Windows x64
 */

#ifndef MASM_OUTPUT_H
#define MASM_OUTPUT_H

#include "core_structures.h"
#include "backend.h"

/* MASM Assembly Context */
typedef struct {
    AssemblyContext *asm_ctx;    /* Reference to assembly context */
    char *output_buffer;         /* Output buffer for assembly code */
    size_t output_capacity;      /* Buffer capacity */
    size_t output_size;          /* Current buffer size */
    int indent_level;            /* Current indentation level */
    int string_counter;          /* Counter for string literal labels */
} MASMContext;

/* MASM Context Management */
MASMContext* masm_context_new(AssemblyContext *asm_ctx);
void masm_context_free(MASMContext *ctx);

/* MASM Assembly Generation */
Bool masm_generate_header(MASMContext *ctx);
Bool masm_generate_entry_point(MASMContext *ctx);
Bool masm_generate_user_main(MASMContext *ctx);
Bool masm_generate_user_main_function(MASMContext *ctx, ASTNode *ast);
Bool masm_generate_footer(MASMContext *ctx);

/* Function-related MASM Generation */
Bool masm_generate_function_declaration(MASMContext *ctx, ASTNode *node);
Bool masm_generate_function_call(MASMContext *ctx, ASTNode *node);
Bool masm_generate_return_statement(MASMContext *ctx, ASTNode *node);
Bool masm_generate_ast_node(MASMContext *ctx, ASTNode *node);

/* Main MASM Generation Function */
Bool masm_generate_assembly(MASMContext *ctx, const char *filename);
Bool masm_generate_assembly_from_ast(MASMContext *ctx, ASTNode *ast, const char *filename);

/* Utility Functions */
void masm_print_debug_info(MASMContext *ctx);

#endif /* MASM_OUTPUT_H */

/*
 * Intermediate Code Generation Implementation
 * Assembly-centric intermediate code generation for SchismC
 * Based on HolyC's LexStmt2Bin functionality
 */

#include "intermediate.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*
 * Core Intermediate Code Generation Functions
 */

ICGenContext* ic_gen_context_new(CCmpCtrl *cc) {
    ICGenContext *ctx = malloc(sizeof(ICGenContext));
    if (!ctx) return NULL;
    
    memset(ctx, 0, sizeof(ICGenContext));
    ctx->cc = cc;
    ctx->ic_count = 0;
    ctx->reg_count = 0;
    ctx->stack_offset = 0;
    ctx->instruction_pointer = 0;
    
    /* Enable optimizations by default */
    ctx->optimization_enabled = true;
    ctx->optimization_level = 9;  /* Full optimization */
    ctx->dead_code_elimination = true;
    ctx->constant_folding = true;
    ctx->register_optimization = true;
    
    return ctx;
}

void ic_gen_context_free(ICGenContext *ctx) {
    if (!ctx) return;
    
    /* Free all intermediate code instructions */
    CIntermediateCode *ic = ctx->ic_head;
    while (ic) {
        CIntermediateCode *next = ic->base.next;
        ic_free(ic);
        ic = next;
    }
    
    free(ctx);
}

CIntermediateCode* ic_gen_add_instruction(ICGenContext *ctx, ICOperation op, CAsmArg *arg1, CAsmArg *arg2, CAsmArg *result) {
    CIntermediateCode *ic = ic_new(op);
    if (!ic) return NULL;
    
    /* Set up assembly-specific fields */
    ic->ic_flags = 0;
    ic->ic_data = 0;
    ic->ic_line = ctx->cc->last_line_num;
    
    /* Copy arguments */
    if (arg1) {
        ic->arg1.i64_val = (I64)arg1;
        ic->arg1.type = 1;  /* Pointer type */
    }
    if (arg2) {
        ic->arg2.i64_val = (I64)arg2;
        ic->arg2.type = 1;  /* Pointer type */
    }
    if (result) {
        ic->res.i64_val = (I64)result;
        ic->res.type = 1;  /* Pointer type */
    }
    
    /* Set assembly instruction mapping */
    ic->x86_opcode = 0x90;  /* NOP by default */
    ic->opcode_size = 1;
    ic->instruction_size = 1;
    
    /* Add to chain */
    if (!ctx->ic_head) {
        ctx->ic_head = ctx->ic_tail = ic;
    } else {
        ctx->ic_tail->base.next = ic;
        ic->base.last = ctx->ic_tail;
        ctx->ic_tail = ic;
    }
    
    ctx->ic_count++;
    return ic;
}

CIntermediateCode* ic_gen_add_assembly(ICGenContext *ctx, U8 opcode, CAsmArg *arg1, CAsmArg *arg2) {
    CIntermediateCode *ic = ic_new(IC_ASM_INLINE);
    if (!ic) return NULL;
    
    /* Set up assembly instruction */
    ic->x86_opcode = opcode;
    ic->opcode_size = 1;
    
    /* Calculate instruction size */
    ic->instruction_size = calculate_instruction_size(arg1, arg2, opcode);
    
    /* Copy arguments */
    if (arg1) {
        ic->arg1.i64_val = (I64)arg1;
        ic->arg1.type = 1;
    }
    if (arg2) {
        ic->arg2.i64_val = (I64)arg2;
        ic->arg2.type = 1;
    }
    
    /* Add to chain */
    if (!ctx->ic_head) {
        ctx->ic_head = ctx->ic_tail = ic;
    } else {
        ctx->ic_tail->base.next = ic;
        ic->base.last = ctx->ic_tail;
        ctx->ic_tail = ic;
    }
    
    ctx->ic_count++;
    return ic;
}

/*
 * LexStmt2Bin equivalent - main compilation function
 * This is the core of HolyC's statement-to-binary compilation
 */
U8* ic_gen_compile_statement(ICGenContext *ctx, I64 *type, I64 cmp_flags) {
    if (!ctx || !ctx->cc) return NULL;
    
    /* Initialize type */
    if (type) *type = RT_I64;
    
    /* Reset intermediate code chain */
    ctx->ic_head = ctx->ic_tail = NULL;
    ctx->ic_count = 0;
    
    /* Parse statement and generate intermediate code */
    /* TODO: This would call the parser to generate IC instructions */
    
    /* Apply optimization passes if enabled */
    if (ctx->optimization_enabled) {
        /* Pass 0-2: Constant folding and type determination */
        if (ctx->optimization_level >= 2) {
            opt_pass_012(ctx);
        }
        
        /* Pass 3: Register allocation optimization */
        if (ctx->optimization_level >= 3) {
            opt_pass_3(ctx);
        }
        
        /* Pass 4: Memory layout optimization */
        if (ctx->optimization_level >= 4) {
            opt_pass_4(ctx);
        }
        
        /* Pass 5: Dead code elimination */
        if (ctx->optimization_level >= 5) {
            opt_pass_5(ctx);
        }
        
        /* Pass 6: Control flow optimization */
        if (ctx->optimization_level >= 6) {
            opt_pass_6(ctx);
        }
        
        /* Pass 7-9: Assembly generation and final optimization */
        if (ctx->optimization_level >= 7) {
            opt_pass_789(ctx);
        }
    }
    
    /* Generate final assembly code */
    I64 size;
    U8 *assembly = ic_generate_assembly(ctx, &size);
    
    return assembly;
}

/*
 * Optimization Pass 0-2: Constant Folding and Type Determination
 * Based on HolyC's OptPass012
 */
Bool opt_pass_012(ICGenContext *ctx) {
    CIntermediateCode *ic = ctx->ic_head;
    
    while (ic) {
        /* Constant folding for arithmetic operations */
        if (ic->base.ic_code >= IC_ADD && ic->base.ic_code <= IC_MOD) {
            if (ic->arg1.type == 0 && ic->arg2.type == 0) {  /* Both are constants */
                I64 result = 0;
                I64 val1 = ic->arg1.i64_val;
                I64 val2 = ic->arg2.i64_val;
                
                switch (ic->base.ic_code) {
                    case IC_ADD: result = val1 + val2; break;
                    case IC_SUB: result = val1 - val2; break;
                    case IC_MUL: result = val1 * val2; break;
                    case IC_DIV: if (val2 != 0) result = val1 / val2; break;
                    case IC_MOD: if (val2 != 0) result = val1 % val2; break;
                }
                
                /* Replace with constant result */
                ic->base.ic_code = IC_NOP;
                ic->res.i64_val = result;
                ic->res.type = 0;  /* Constant type */
            }
        }
        
        /* Type determination for expressions */
        if (ic->arg1.type == 1 && ic->arg2.type == 1) {  /* Both are pointers */
            /* Determine result type based on operation */
            ic->res.type = 1;  /* Pointer result */
        }
        
        ic = ic->base.next;
    }
    
    return true;
}

/*
 * Optimization Pass 3: Register Allocation Optimization
 * Based on HolyC's OptPass3
 */
Bool opt_pass_3(ICGenContext *ctx) {
    CIntermediateCode *ic = ctx->ic_head;
    
    while (ic) {
        /* Register allocation for variables */
        if (ic->base.ic_code == IC_ASSIGN || ic->base.ic_code == IC_LOAD) {
            /* Allocate register for result */
            if (ctx->reg_count < MAX_X86_REGS) {
                X86Register reg = (X86Register)(REG_RAX + ctx->reg_count);
                ctx->allocated_regs[ctx->reg_count] = reg;
                ctx->reg_count++;
                
                /* Mark register as allocated in IC */
                ic->reg_alloc[ic->reg_count] = reg;
                ic->reg_count++;
                ic->regs_allocated = true;
            }
        }
        
        ic = ic->base.next;
    }
    
    return true;
}

/*
 * Optimization Pass 4: Memory Layout Optimization
 * Based on HolyC's OptPass4
 */
Bool opt_pass_4(ICGenContext *ctx) {
    CIntermediateCode *ic = ctx->ic_head;
    
    while (ic) {
        /* Optimize memory access patterns */
        if (ic->base.ic_code == IC_STORE || ic->base.ic_code == IC_LOAD) {
            /* Calculate optimal stack offset */
            ic->stack_offset = ctx->stack_offset;
            ctx->stack_offset += 8;  /* Assume 64-bit alignment */
        }
        
        ic = ic->base.next;
    }
    
    return true;
}

/*
 * Optimization Pass 5: Dead Code Elimination
 * Based on HolyC's OptPass5
 */
Bool opt_pass_5(ICGenContext *ctx) {
    CIntermediateCode *ic = ctx->ic_head;
    CIntermediateCode *prev = NULL;
    
    while (ic) {
        if (ic_is_dead(ic)) {
            /* Remove dead instruction */
            if (prev) {
                prev->base.next = ic->base.next;
                if (ic->base.next) {
                    ic->base.next->base.last = prev;
                } else {
                    ctx->ic_tail = prev;
                }
            } else {
                ctx->ic_head = ic->base.next;
                if (ctx->ic_head) {
                    ctx->ic_head->base.last = NULL;
                }
            }
            
            CIntermediateCode *dead = ic;
            ic = ic->base.next;
            ic_free(dead);
            ctx->ic_count--;
        } else {
            prev = ic;
            ic = ic->base.next;
        }
    }
    
    return true;
}

/*
 * Optimization Pass 6: Control Flow Optimization
 * Based on HolyC's OptPass6
 */
Bool opt_pass_6(ICGenContext *ctx) {
    CIntermediateCode *ic = ctx->ic_head;
    
    while (ic) {
        /* Optimize jump instructions */
        if (ic->base.ic_code == IC_JUMP || ic->base.ic_code == IC_JUMP_TRUE || ic->base.ic_code == IC_JUMP_FALSE) {
            /* TODO: Implement jump optimization */
        }
        
        ic = ic->base.next;
    }
    
    return true;
}

/*
 * Optimization Pass 7-9: Assembly Generation and Final Optimization
 * Based on HolyC's OptPass789A
 */
Bool opt_pass_789(ICGenContext *ctx) {
    CIntermediateCode *ic = ctx->ic_head;
    
    while (ic) {
        /* Generate assembly bytes for instruction */
        if (ic->base.ic_code != IC_NOP) {
            U8 *assembly = malloc(ic->instruction_size);
            if (assembly) {
                I64 size;
                CAsmArg *arg1 = (CAsmArg*)ic->arg1.i64_val;
                CAsmArg *arg2 = (CAsmArg*)ic->arg2.i64_val;
                
                if (encode_x86_instruction(arg1, arg2, ic->x86_opcode, assembly, &size)) {
                    ic->assembly_bytes = assembly;
                    ic->assembly_size = size;
                    ic->assembly_generated = true;
                } else {
                    free(assembly);
                }
            }
        }
        
        ic = ic->base.next;
    }
    
    return true;
}

/*
 * Utility Functions
 */

Bool ic_is_dead(CIntermediateCode *ic) {
    /* Check if instruction result is never used */
    if (ic->base.ic_code == IC_NOP) return true;
    
    /* TODO: Implement more sophisticated dead code detection */
    return false;
}

I64 ic_calculate_cost(CIntermediateCode *ic) {
    /* Calculate cost of instruction for optimization */
    switch (ic->base.ic_code) {
        case IC_NOP: return 0;
        case IC_ADD: case IC_SUB: return 1;
        case IC_MUL: case IC_DIV: return 3;
        case IC_LOAD: case IC_STORE: return 2;
        default: return 1;
    }
}

/*
 * Assembly Generation from Intermediate Code
 */

U8* ic_generate_assembly(ICGenContext *ctx, I64 *size) {
    if (!ctx || !size) return NULL;
    
    /* Calculate total size needed */
    I64 total_size = 0;
    CIntermediateCode *ic = ctx->ic_head;
    while (ic) {
        total_size += ic->instruction_size;
        ic = ic->base.next;
    }
    
    /* Allocate output buffer */
    U8 *output = malloc(total_size);
    if (!output) return NULL;
    
    /* Generate assembly for each instruction */
    I64 offset = 0;
    ic = ctx->ic_head;
    while (ic) {
        if (ic->assembly_generated && ic->assembly_bytes) {
            memcpy(output + offset, ic->assembly_bytes, ic->assembly_size);
            offset += ic->assembly_size;
        }
        ic = ic->base.next;
    }
    
    *size = offset;
    return output;
}

/*
 * AOT Compilation Support
 */

Bool ic_prepare_aot(ICGenContext *ctx, CAOT *aot) {
    if (!ctx || !aot) return false;
    
    /* Prepare intermediate code for AOT compilation */
    CIntermediateCode *ic = ctx->ic_head;
    while (ic) {
        /* Mark instruction for AOT compilation */
        ic->ic_flags |= ICF_AOT_COMPILE;
        ic = ic->base.next;
    }
    
    return true;
}

Bool ic_resolve_symbols(ICGenContext *ctx, CAOT *aot) {
    if (!ctx || !aot) return false;
    
    /* Resolve symbols in intermediate code */
    CIntermediateCode *ic = ctx->ic_head;
    while (ic) {
        /* TODO: Implement symbol resolution */
        ic = ic->base.next;
    }
    
    return true;
}

/*
 * AST-to-Intermediate Code Conversion
 * Convert AST nodes into optimized intermediate code
 */

/* Main AST-to-IC conversion function */
Bool ic_gen_from_ast(ICGenContext *ctx, ASTNode *ast) {
    if (!ctx || !ast) return false;
    
    printf("DEBUG: Converting AST to intermediate code...\n");
    printf("  - AST root type: %d\n", ast->type);
    
    /* Reset IC chain */
    ctx->ic_head = ctx->ic_tail = NULL;
    ctx->ic_count = 0;
    
    /* Convert AST nodes to intermediate code */
    ASTNode *child = ast->children;
    while (child) {
        if (!ic_gen_ast_node(ctx, child)) {
            printf("ERROR: Failed to convert AST node type %d\n", child->type);
            return false;
        }
        child = child->next;
    }
    
    printf("DEBUG: Generated %lld intermediate code instructions\n", ctx->ic_count);
    return true;
}

/* Convert individual AST node to intermediate code */
Bool ic_gen_ast_node(ICGenContext *ctx, ASTNode *node) {
    if (!ctx || !node) return false;
    
    switch (node->type) {
        case NODE_STRING:
            return ic_gen_string_literal(ctx, node);
        case NODE_INTEGER:
            return ic_gen_integer_literal(ctx, node);
        case NODE_FLOAT:
            return ic_gen_float_literal(ctx, node);
        case NODE_CHAR:
            return ic_gen_char_literal(ctx, node);
        case NODE_IDENTIFIER:
            return ic_gen_identifier(ctx, node);
        case NODE_BINARY_OP:
            return ic_gen_binary_operation(ctx, node);
        case NODE_UNARY_OP:
            return ic_gen_unary_operation(ctx, node);
        case NODE_CALL:
            return ic_gen_function_call(ctx, node);
        case NODE_ASSIGNMENT:
            return ic_gen_assignment(ctx, node);
        case NODE_VARIABLE:
            return ic_gen_variable_declaration(ctx, node);
        case NODE_FUNCTION:
            return ic_gen_function_declaration(ctx, node);
        case NODE_IF:
            return ic_gen_if_statement(ctx, node);
        case NODE_WHILE:
            return ic_gen_while_statement(ctx, node);
        case NODE_FOR:
            return ic_gen_for_statement(ctx, node);
        case NODE_RETURN:
            return ic_gen_return_statement(ctx, node);
        case NODE_BLOCK:
            return ic_gen_block_statement(ctx, node);
        case NODE_ASM_BLOCK:
            return ic_gen_assembly_block(ctx, node);
        default:
            printf("WARNING: Unhandled AST node type: %d\n", node->type);
            return true; /* Don't fail on unhandled types */
    }
}

/* Convert string literal to intermediate code */
Bool ic_gen_string_literal(ICGenContext *ctx, ASTNode *node) {
    if (!ctx || !node || node->type != NODE_STRING) return false;
    
    printf("DEBUG: Converting string literal: %s\n", node->data.literal.str_value);
    
    /* Create assembly argument for string literal */
    CAsmArg *str_arg = asmarg_new();
    if (!str_arg) return false;
    
    /* Set up string literal argument */
    str_arg->is_immediate = true;
    str_arg->num.i64_val = (I64)node->data.literal.str_value;
    str_arg->size = 8; /* 64-bit pointer */
    
    /* Generate intermediate code for string literal */
    /* In HolyC, string literals are automatically printed */
    CIntermediateCode *ic = ic_gen_add_instruction(ctx, IC_PRINT, str_arg, NULL, NULL);
    if (!ic) return false;
    
    /* Set up assembly instruction for printf call */
    ic->x86_opcode = 0xE8; /* CALL instruction */
    ic->instruction_size = 5; /* CALL rel32 */
    
    return true;
}

/* Convert integer literal to intermediate code */
Bool ic_gen_integer_literal(ICGenContext *ctx, ASTNode *node) {
    if (!ctx || !node || node->type != NODE_INTEGER) return false;
    
    printf("DEBUG: Converting integer literal: %lld\n", node->data.literal.i64_value);
    
    /* Create assembly argument for integer literal */
    CAsmArg *int_arg = asmarg_new();
    if (!int_arg) return false;
    
    /* Set up integer literal argument */
    int_arg->is_immediate = true;
    int_arg->num.i64_val = node->data.literal.i64_value;
    int_arg->size = 8; /* 64-bit integer */
    
    /* Generate intermediate code for integer literal */
    /* In HolyC, integer literals are automatically printed */
    CIntermediateCode *ic = ic_gen_add_instruction(ctx, IC_PRINT, int_arg, NULL, NULL);
    if (!ic) return false;
    
    /* Set up assembly instruction for printf call */
    ic->x86_opcode = 0xE8; /* CALL instruction */
    ic->instruction_size = 5; /* CALL rel32 */
    
    return true;
}

/* Convert float literal to intermediate code */
Bool ic_gen_float_literal(ICGenContext *ctx, ASTNode *node) {
    if (!ctx || !node || node->type != NODE_FLOAT) return false;
    
    printf("DEBUG: Converting float literal: %f\n", node->data.literal.f64_value);
    
    /* Create assembly argument for float literal */
    CAsmArg *float_arg = asmarg_new();
    if (!float_arg) return false;
    
    /* Set up float literal argument */
    float_arg->is_immediate = true;
    float_arg->num.f64_val = node->data.literal.f64_value;
    float_arg->size = 8; /* 64-bit float */
    
    /* Generate intermediate code for float literal */
    /* In HolyC, float literals are automatically printed */
    CIntermediateCode *ic = ic_gen_add_instruction(ctx, IC_PRINT, float_arg, NULL, NULL);
    if (!ic) return false;
    
    /* Set up assembly instruction for printf call */
    ic->x86_opcode = 0xE8; /* CALL instruction */
    ic->instruction_size = 5; /* CALL rel32 */
    
    return true;
}

/* Convert character literal to intermediate code */
Bool ic_gen_char_literal(ICGenContext *ctx, ASTNode *node) {
    if (!ctx || !node || node->type != NODE_CHAR) return false;
    
    printf("DEBUG: Converting char literal: '%c'\n", node->data.literal.char_value);
    
    /* Create assembly argument for character literal */
    CAsmArg *char_arg = asmarg_new();
    if (!char_arg) return false;
    
    /* Set up character literal argument */
    char_arg->is_immediate = true;
    char_arg->num.i64_val = node->data.literal.char_value;
    char_arg->size = 1; /* 8-bit character */
    
    /* Generate intermediate code for character literal */
    /* In HolyC, character literals are automatically printed */
    CIntermediateCode *ic = ic_gen_add_instruction(ctx, IC_PRINT, char_arg, NULL, NULL);
    if (!ic) return false;
    
    /* Set up assembly instruction for printf call */
    ic->x86_opcode = 0xE8; /* CALL instruction */
    ic->instruction_size = 5; /* CALL rel32 */
    
    return true;
}

/* Convert identifier to intermediate code */
Bool ic_gen_identifier(ICGenContext *ctx, ASTNode *node) {
    if (!ctx || !node || node->type != NODE_IDENTIFIER) return false;
    
    printf("DEBUG: Converting identifier: %s\n", node->data.identifier.name);
    
    /* TODO: Implement identifier handling */
    /* For now, just create a NOP instruction */
    CIntermediateCode *ic = ic_gen_add_instruction(ctx, IC_NOP, NULL, NULL, NULL);
    return ic != NULL;
}

/* Placeholder implementations for other AST node types */
Bool ic_gen_binary_operation(ICGenContext *ctx, ASTNode *node) {
    if (!ctx || !node || node->type != NODE_BINARY_OP) return false;
    
    printf("DEBUG: Generating intermediate code for binary operation: %d\n", node->data.binary_op.op);
    
    /* First, try constant folding optimization */
    if (ctx->constant_folding && ic_is_constant_expression(node)) {
        ASTNode *folded = ic_fold_constant_expression(node);
        if (folded) {
            printf("DEBUG: Constant folded binary operation\n");
            /* Replace the binary operation with the folded constant */
            node->type = folded->type;
            node->data = folded->data;
            return ic_gen_ast_node(ctx, node);
        }
    }
    
    /* Generate intermediate code for left operand */
    if (!ic_gen_ast_node(ctx, node->data.binary_op.left)) {
        printf("ERROR: Failed to generate IC for left operand\n");
        return false;
    }
    
    /* Generate intermediate code for right operand */
    if (!ic_gen_ast_node(ctx, node->data.binary_op.right)) {
        printf("ERROR: Failed to generate IC for right operand\n");
        return false;
    }
    
    /* Create intermediate code instruction for the binary operation */
    CIntermediateCode *ic = ic_gen_add_instruction(ctx, IC_ADD, NULL, NULL, NULL);
    if (!ic) {
        printf("ERROR: Failed to create binary operation IC\n");
        return false;
    }
    
    /* Set operation details based on operator type */
    switch (node->data.binary_op.op) {
        case BINOP_ADD:
            ic->base.ic_code = IC_ADD;
            break;
        case BINOP_SUB:
            ic->base.ic_code = IC_SUB;
            break;
        case BINOP_MUL:
            ic->base.ic_code = IC_MUL;
            break;
        case BINOP_DIV:
            ic->base.ic_code = IC_DIV;
            break;
        case BINOP_MOD:
            ic->base.ic_code = IC_MOD;
            break;
        default:
            printf("ERROR: Unsupported binary operator: %d\n", node->data.binary_op.op);
            return false;
    }
    
    /* Set register allocation info */
    ic->reg_count = 3; /* left, right, result registers */
    ic->reg_alloc[0] = ctx->reg_count++;  /* Temporary register for left operand */
    ic->reg_alloc[1] = ctx->reg_count++; /* Temporary register for right operand */
    ic->reg_alloc[2] = ctx->reg_count++; /* Result register */
    
    printf("DEBUG: Binary operation IC generated successfully\n");
    return true;
}

Bool ic_gen_unary_operation(ICGenContext *ctx, ASTNode *node) {
    if (!ctx || !node || node->type != NODE_UNARY_OP) return false;
    
    printf("DEBUG: Generating intermediate code for unary operation: %d\n", node->data.unary_op.op);
    
    /* First, try constant folding optimization */
    if (ctx->constant_folding && ic_is_constant_expression(node)) {
        ASTNode *folded = ic_fold_constant_expression(node);
        if (folded) {
            printf("DEBUG: Constant folded unary operation\n");
            /* Replace the unary operation with the folded constant */
            node->type = folded->type;
            node->data = folded->data;
            return ic_gen_ast_node(ctx, node);
        }
    }
    
    /* Generate intermediate code for operand */
    if (!ic_gen_ast_node(ctx, node->data.unary_op.operand)) {
        printf("ERROR: Failed to generate IC for unary operand\n");
        return false;
    }
    
    /* Create intermediate code instruction for the unary operation */
    CIntermediateCode *ic = ic_gen_add_instruction(ctx, IC_PRINT, NULL, NULL, NULL); // Use IC_PRINT as placeholder
    if (!ic) {
        printf("ERROR: Failed to create unary operation IC\n");
        return false;
    }
    
    /* Set operation details based on operator type */
    switch (node->data.unary_op.op) {
        case UNOP_NOT: ic->base.ic_code = IC_PRINT; break; /* Placeholder - need IC_UNARY_NOT */
        case UNOP_PLUS: ic->base.ic_code = IC_PRINT; break; /* No operation needed */
        case UNOP_MINUS: ic->base.ic_code = IC_PRINT; break; /* Placeholder - need IC_UNARY_MINUS */
        case UNOP_BITNOT: ic->base.ic_code = IC_PRINT; break; /* Placeholder - need IC_UNARY_BITNOT */
        case UNOP_INC: ic->base.ic_code = IC_PRINT; break; /* Placeholder - need IC_UNARY_INC */
        case UNOP_DEC: ic->base.ic_code = IC_PRINT; break; /* Placeholder - need IC_UNARY_DEC */
        case UNOP_ADDR: ic->base.ic_code = IC_PRINT; break; /* Placeholder - need IC_UNARY_ADDR */
        case UNOP_DEREF: ic->base.ic_code = IC_PRINT; break; /* Placeholder - need IC_UNARY_DEREF */
        default:
            printf("ERROR: Unsupported unary operator: %d\n", node->data.unary_op.op);
            return false;
    }
    
    /* Set register allocation info */
    ic->reg_count = 2; /* operand, result registers */
    ic->reg_alloc[0] = ctx->reg_count++;  /* Temporary register for operand */
    ic->reg_alloc[1] = ctx->reg_count++;  /* Result register */
    
    printf("DEBUG: Unary operation IC generated successfully\n");
    return true;
}

/*
 * Constant Folding and Expression Evaluation
 */

Bool ic_is_constant_expression(ASTNode *node) {
    if (!node) return false;
    
    switch (node->type) {
        case NODE_INTEGER:
        case NODE_FLOAT:
        case NODE_CHAR:
            return true;
            
        case NODE_BINARY_OP:
            /* Binary operation is constant if both operands are constant */
            return ic_is_constant_expression(node->data.binary_op.left) &&
                   ic_is_constant_expression(node->data.binary_op.right);
                   
        case NODE_UNARY_OP:
            /* Unary operation is constant if operand is constant */
            return ic_is_constant_expression(node->data.unary_op.operand);
            
        default:
            return false;
    }
}

ASTNode* ic_fold_constant_expression(ASTNode *node) {
    if (!node) return NULL;
    
    switch (node->type) {
        case NODE_INTEGER:
        case NODE_FLOAT:
        case NODE_CHAR:
            /* Already a constant, return a copy */
            return ic_copy_ast_node(node);
            
        case NODE_BINARY_OP: {
            /* Fold binary operations */
            ASTNode *left = ic_fold_constant_expression(node->data.binary_op.left);
            ASTNode *right = ic_fold_constant_expression(node->data.binary_op.right);
            
            if (!left || !right) {
                if (left) ast_node_free(left);
                if (right) ast_node_free(right);
                return NULL;
            }
            
            /* Evaluate the operation */
            ASTNode *result = ic_evaluate_binary_operation(left, right, node->data.binary_op.op);
            
            /* Clean up temporary nodes */
            ast_node_free(left);
            ast_node_free(right);
            
            return result;
        }
        
        case NODE_UNARY_OP: {
            /* Fold unary operations */
            ASTNode *operand = ic_fold_constant_expression(node->data.unary_op.operand);
            
            if (!operand) return NULL;
            
            /* Evaluate the operation */
            ASTNode *result = ic_evaluate_unary_operation(operand, node->data.unary_op.op);
            
            /* Clean up temporary node */
            ast_node_free(operand);
            
            return result;
        }
        
        default:
            return NULL;
    }
}

ASTNode* ic_evaluate_binary_operation(ASTNode *left, ASTNode *right, BinaryOpType op) {
    if (!left || !right) return NULL;
    
    /* For now, only handle integer operations */
    if (left->type != NODE_INTEGER || right->type != NODE_INTEGER) {
        printf("DEBUG: Only integer constant folding supported for now\n");
        return NULL;
    }
    
    I64 left_val = left->data.literal.i64_value;
    I64 right_val = right->data.literal.i64_value;
    I64 result_val = 0;
    
    switch (op) {
        case BINOP_ADD:
            result_val = left_val + right_val;
            break;
        case BINOP_SUB:
            result_val = left_val - right_val;
            break;
        case BINOP_MUL:
            result_val = left_val * right_val;
            break;
        case BINOP_DIV:
            if (right_val == 0) {
                printf("ERROR: Division by zero in constant folding\n");
                return NULL;
            }
            result_val = left_val / right_val;
            break;
        case BINOP_MOD:
            if (right_val == 0) {
                printf("ERROR: Modulo by zero in constant folding\n");
                return NULL;
            }
            result_val = left_val % right_val;
            break;
        default:
            printf("DEBUG: Unsupported binary operation for constant folding: %d\n", op);
            return NULL;
    }
    
    /* Create result node */
    ASTNode *result = ast_node_new(NODE_INTEGER, left->line, left->column);
    if (!result) return NULL;
    
    result->data.literal.i64_value = result_val;
    
    printf("DEBUG: Constant folded %lld %s %lld = %lld\n", 
           left_val, ic_get_operator_string(op), right_val, result_val);
    
    return result;
}

ASTNode* ic_evaluate_unary_operation(ASTNode *operand, UnaryOpType op) {
    if (!operand) return NULL;
    
    /* For now, only handle integer operations */
    if (operand->type != NODE_INTEGER) {
        printf("DEBUG: Only integer unary constant folding supported for now\n");
        return NULL;
    }
    
    I64 operand_val = operand->data.literal.i64_value;
    I64 result_val = 0;
    
    switch (op) {
        case UNOP_PLUS:
            result_val = +operand_val;
            break;
        case UNOP_MINUS:
            result_val = -operand_val;
            break;
        case UNOP_NOT:
            result_val = !operand_val;
            break;
        case UNOP_BITNOT:
            result_val = ~operand_val;
            break;
        case UNOP_INC:
            result_val = operand_val + 1;
            break;
        case UNOP_DEC:
            result_val = operand_val - 1;
            break;
        case UNOP_ADDR:
        case UNOP_DEREF:
            /* Address and dereference operations cannot be folded at compile time */
            printf("DEBUG: Address/dereference operations cannot be constant folded\n");
            return NULL;
        default:
            printf("DEBUG: Unsupported unary operation for constant folding: %d\n", op);
            return NULL;
    }
    
    /* Create result node */
    ASTNode *result = ast_node_new(NODE_INTEGER, operand->line, operand->column);
    if (!result) return NULL;
    
    result->data.literal.i64_value = result_val;
    
    printf("DEBUG: Constant folded %s%lld = %lld\n", 
           ic_get_unary_operator_string(op), operand_val, result_val);
    
    return result;
}

ASTNode* ic_copy_ast_node(ASTNode *node) {
    if (!node) return NULL;
    
    ASTNode *copy = ast_node_new(node->type, node->line, node->column);
    if (!copy) return NULL;
    
    /* Copy node-specific data */
    switch (node->type) {
        case NODE_INTEGER:
            copy->data.literal.i64_value = node->data.literal.i64_value;
            break;
        case NODE_FLOAT:
            copy->data.literal.f64_value = node->data.literal.f64_value;
            break;
        case NODE_CHAR:
            copy->data.literal.char_value = node->data.literal.char_value;
            break;
        case NODE_STRING:
            if (node->data.literal.str_value) {
                I64 len = strlen((char*)node->data.literal.str_value);
                copy->data.literal.str_value = (U8*)malloc(len + 1);
                if (copy->data.literal.str_value) {
                    strcpy((char*)copy->data.literal.str_value, (char*)node->data.literal.str_value);
                }
            }
            break;
        default:
            /* For complex nodes, just copy the basic structure */
            break;
    }
    
    return copy;
}

const char* ic_get_operator_string(BinaryOpType op) {
    switch (op) {
        case BINOP_ADD: return "+";
        case BINOP_SUB: return "-";
        case BINOP_MUL: return "*";
        case BINOP_DIV: return "/";
        case BINOP_MOD: return "%";
        case BINOP_AND: return "&";
        case BINOP_OR: return "|";
        case BINOP_XOR: return "^";
        case BINOP_SHL: return "<<";
        case BINOP_SHR: return ">>";
        case BINOP_EQ: return "==";
        case BINOP_NE: return "!=";
        case BINOP_LT: return "<";
        case BINOP_LE: return "<=";
        case BINOP_GT: return ">";
        case BINOP_GE: return ">=";
        case BINOP_AND_AND: return "&&";
        case BINOP_OR_OR: return "||";
        case BINOP_XOR_XOR: return "^^";
        case BINOP_ASSIGN: return "=";
        case BINOP_ADD_ASSIGN: return "+=";
        case BINOP_SUB_ASSIGN: return "-=";
        case BINOP_MUL_ASSIGN: return "*=";
        case BINOP_DIV_ASSIGN: return "/=";
        case BINOP_MOD_ASSIGN: return "%=";
        case BINOP_AND_ASSIGN: return "&=";
        case BINOP_OR_ASSIGN: return "|=";
        case BINOP_XOR_ASSIGN: return "^=";
        case BINOP_SHL_ASSIGN: return "<<=";
        case BINOP_SHR_ASSIGN: return ">>=";
        case BINOP_RANGE: return "..";
        case BINOP_DOT_DOT: return "..";
        default: return "?";
    }
}

const char* ic_get_unary_operator_string(UnaryOpType op) {
    switch (op) {
        case UNOP_PLUS: return "+";
        case UNOP_MINUS: return "-";
        case UNOP_NOT: return "!";
        case UNOP_BITNOT: return "~";
        case UNOP_INC: return "++";
        case UNOP_DEC: return "--";
        case UNOP_DEREF: return "*";
        case UNOP_ADDR: return "&";
        default: return "?";
    }
}

Bool ic_gen_function_call(ICGenContext *ctx, ASTNode *node) {
    if (!ctx || !node || node->type != NODE_CALL) return false;
    
    printf("DEBUG: Generating intermediate code for function call: %s\n", 
           node->data.call.name ? (char*)node->data.call.name : "unknown");
    
    /* Generate intermediate code for function arguments if present */
    if (node->data.call.arguments) {
        printf("DEBUG: Processing function call arguments\n");
        if (!ic_gen_ast_node(ctx, node->data.call.arguments)) {
            printf("ERROR: Failed to generate intermediate code for function call arguments\n");
            return false;
        }
    }
    
    /* Generate function call instruction */
    CIntermediateCode *ic = ic_gen_add_instruction(ctx, IC_CALL, 
        (void*)node->data.call.name, NULL, NULL);
    if (!ic) {
        printf("ERROR: Failed to add function call instruction\n");
        return false;
    }
    
    printf("DEBUG: Function call intermediate code generated successfully\n");
    return true;
}

Bool ic_gen_assignment(ICGenContext *ctx, ASTNode *node) {
    printf("DEBUG: Assignment not yet implemented\n");
    return true;
}

Bool ic_gen_variable_declaration(ICGenContext *ctx, ASTNode *node) {
    printf("DEBUG: Variable declaration not yet implemented\n");
    return true;
}

Bool ic_gen_function_declaration(ICGenContext *ctx, ASTNode *node) {
    if (!ctx || !node || node->type != NODE_FUNCTION) return false;
    
    printf("DEBUG: Generating intermediate code for function: %s\n", 
           node->data.function.name ? (char*)node->data.function.name : "unknown");
    
    /* Generate intermediate code for function body */
    if (node->data.function.body) {
        if (!ic_gen_block_statement(ctx, node->data.function.body)) {
            printf("ERROR: Failed to generate intermediate code for function body\n");
            return false;
        }
    }
    
    printf("DEBUG: Function declaration intermediate code generated successfully\n");
    return true;
}

Bool ic_gen_if_statement(ICGenContext *ctx, ASTNode *node) {
    printf("DEBUG: If statement not yet implemented\n");
    return true;
}

Bool ic_gen_while_statement(ICGenContext *ctx, ASTNode *node) {
    printf("DEBUG: While statement not yet implemented\n");
    return true;
}

Bool ic_gen_for_statement(ICGenContext *ctx, ASTNode *node) {
    printf("DEBUG: For statement not yet implemented\n");
    return true;
}

Bool ic_gen_return_statement(ICGenContext *ctx, ASTNode *node) {
    if (!ctx || !node || node->type != NODE_RETURN) return false;
    
    printf("DEBUG: Generating intermediate code for return statement\n");
    printf("DEBUG: Return value: %lld, Expression: %p\n", 
           node->data.return_stmt.return_value, 
           node->data.return_stmt.expression);
    
    /* Generate intermediate code for return expression if present */
    if (node->data.return_stmt.expression) {
        printf("DEBUG: Processing return expression\n");
        if (!ic_gen_ast_node(ctx, node->data.return_stmt.expression)) {
            printf("ERROR: Failed to generate intermediate code for return expression\n");
            return false;
        }
        printf("DEBUG: Return expression processed successfully\n");
    } else {
        /* Simple return value (like return 42;) */
        if (node->data.return_stmt.return_value != 0) {
            /* Generate intermediate code for the return value */
            CIntermediateCode *ic = ic_gen_add_instruction(ctx, IC_RETURN_VAL, 
                (void*)node->data.return_stmt.return_value, NULL, NULL);
            if (!ic) {
                printf("ERROR: Failed to add return value instruction\n");
                return false;
            }
            printf("DEBUG: Added return value instruction: %lld\n", node->data.return_stmt.return_value);
        } else {
            /* No return value, just return */
            CIntermediateCode *ic = ic_gen_add_instruction(ctx, IC_RETURN, NULL, NULL, NULL);
            if (!ic) {
                printf("ERROR: Failed to add return instruction\n");
                return false;
            }
            printf("DEBUG: Added return instruction\n");
        }
    }
    
    printf("DEBUG: Return statement intermediate code generated successfully\n");
    return true;
}

Bool ic_gen_block_statement(ICGenContext *ctx, ASTNode *node) {
    if (!ctx || !node || node->type != NODE_BLOCK) return false;
    
    printf("DEBUG: Generating intermediate code for block statement with %d statements\n", 
           node->data.block.statement_count);
    
    /* Generate intermediate code for each statement in the block */
    ASTNode *current_stmt = node->data.block.statements;
    while (current_stmt) {
        printf("DEBUG: Processing block statement, type: %d\n", current_stmt->type);
        
        /* Generate intermediate code for this statement */
        if (!ic_gen_ast_node(ctx, current_stmt)) {
            printf("ERROR: Failed to generate intermediate code for block statement\n");
            return false;
        }
        
        /* Move to next statement */
        current_stmt = current_stmt->next;
    }
    
    printf("DEBUG: Block statement intermediate code generated successfully\n");
    return true;
}

Bool ic_gen_assembly_block(ICGenContext *ctx, ASTNode *node) {
    printf("DEBUG: Assembly block not yet implemented\n");
    return true;
}

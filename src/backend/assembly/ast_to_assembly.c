/*
 * AST-to-Assembly Direct Converter
 * Generates x86-64 assembly directly from AST nodes
 * This bypasses the intermediate code layer for direct assembly generation
 */

#include "backend.h"
#include "parser.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * AST-to-Assembly Conversion Functions
 */

Bool ast_to_assembly_generate(AssemblyContext *ctx, ASTNode *ast) {
    if (!ctx || !ast) return false;
    
    printf("DEBUG: Starting AST-to-assembly generation...\n");
    printf("  - AST root type: %d\n", ast->type);
    
    /* Initialize assembly context for direct generation */
    ctx->buffer_size = 0;
    ctx->instruction_pointer = 0;
    ctx->reg_count = 0;
    ctx->stack_offset = 0;
    
    /* Generate assembly from AST children */
    ASTNode *child = ast->children;
    I64 statement_count = 0;
    
    while (child) {
        printf("DEBUG: Processing AST child type %d\n", child->type);
        
        if (!ast_to_assembly_node(ctx, child)) {
            printf("ERROR: Failed to generate assembly for AST node type %d\n", child->type);
            return false;
        }
        
        statement_count++;
        child = child->next;
    }
    
    printf("DEBUG: Generated assembly for %lld statements\n", statement_count);
    return true;
}

Bool ast_to_assembly_node(AssemblyContext *ctx, ASTNode *node) {
    if (!ctx || !node) return false;
    
    switch (node->type) {
        case NODE_STRING:
            return ast_to_assembly_string_literal(ctx, node);
            
        case NODE_INTEGER:
            return ast_to_assembly_integer_literal(ctx, node);
            
        case NODE_FLOAT:
            return ast_to_assembly_float_literal(ctx, node);
            
        case NODE_CHAR:
            return ast_to_assembly_char_literal(ctx, node);
            
        case NODE_BINARY_OP:
            return ast_to_assembly_binary_operation(ctx, node);
            
        case NODE_UNARY_OP:
            return ast_to_assembly_unary_operation(ctx, node);
            
        case NODE_CALL:
            return ast_to_assembly_function_call(ctx, node);
            
        case NODE_ASSIGNMENT:
            return ast_to_assembly_assignment(ctx, node);
            
        case NODE_VARIABLE:
            // Check if this is a variable reference (used in expressions) or declaration
            if (node->data.identifier.stack_offset >= 0) {
                // This is a variable reference in an expression
                return ast_to_assembly_variable_reference(ctx, node);
            } else {
                // This is a variable declaration
                return ast_to_assembly_variable_declaration(ctx, node);
            }
            
        case NODE_FUNCTION:
            return ast_to_assembly_function_declaration(ctx, node);
            
        case NODE_IF_STMT:
            return ast_to_assembly_if_statement(ctx, node);
            
        case NODE_WHILE_STMT:
            return ast_to_assembly_while_statement(ctx, node);
            
        case NODE_FOR_STMT:
            return ast_to_assembly_for_statement(ctx, node);
            
        case NODE_RETURN:
            return ast_to_assembly_return_statement(ctx, node);
            
        case NODE_BLOCK:
            return ast_to_assembly_block_statement(ctx, node);
            
        case NODE_ASM_BLOCK:
            return ast_to_assembly_inline_assembly(ctx, node);
            
        default:
            printf("WARNING: Unhandled AST node type: %d\n", node->type);
            return true;  /* Don't fail on unknown nodes */
    }
}

/*
 * Literal Assembly Generation
 */

Bool ast_to_assembly_string_literal(AssemblyContext *ctx, ASTNode *node) {
    if (!ctx || !node || node->type != NODE_STRING) return false;
    
    printf("DEBUG: Generating assembly for string literal: %s\n", node->data.literal.str_value);
    
    /* Generate assembly for string literal (equivalent to HolyC's Print() function) */
    /* This generates a call to printf with the string as argument */
    
    /* Allocate registers for function call */
    X86Register arg_reg = asm_allocate_register(ctx, 8);
    if (arg_reg == REG_NONE) {
        printf("ERROR: Failed to allocate register for string argument\n");
        return false;
    }
    
    /* Setup arguments for printf call */
    CAsmArg format_arg = {0};
    CAsmArg string_arg = {0};
    
    /* Load string address into RCX (first argument register) */
    asm_setup_immediate_arg(ctx, &string_arg, (I64)node->data.literal.str_value);
    
    /* Generate LEA instruction to load string address */
    if (!asm_generate_lea(ctx, &format_arg, &string_arg)) {
        printf("ERROR: Failed to generate LEA instruction for string\n");
        asm_free_register(ctx, arg_reg);
        return false;
    }
    
    /* Generate CALL instruction to printf */
    if (!asm_generate_call(ctx, 0)) {  /* printf address will be resolved later */
        printf("ERROR: Failed to generate CALL instruction\n");
        asm_free_register(ctx, arg_reg);
        return false;
    }
    
    asm_free_register(ctx, arg_reg);
    return true;
}

Bool ast_to_assembly_integer_literal(AssemblyContext *ctx, ASTNode *node) {
    if (!ctx || !node || node->type != NODE_INTEGER) return false;
    
    printf("DEBUG: Generating assembly for integer literal: %lld\n", node->data.literal.i64_value);
    
    /* Generate assembly for integer literal (equivalent to HolyC's Print() function) */
    /* This generates a call to printf with the integer as argument */
    
    /* Allocate registers for function call */
    X86Register arg_reg = asm_allocate_register(ctx, 8);
    if (arg_reg == REG_NONE) {
        printf("ERROR: Failed to allocate register for integer argument\n");
        return false;
    }
    
    /* Setup arguments for printf call */
    CAsmArg format_arg = {0};
    CAsmArg int_arg = {0};
    
    /* Load integer value into RCX (first argument register) */
    asm_setup_immediate_arg(ctx, &int_arg, node->data.literal.i64_value);
    
    /* Generate MOV instruction to load integer */
    if (!asm_generate_mov(ctx, &format_arg, &int_arg)) {
        printf("ERROR: Failed to generate MOV instruction for integer\n");
        asm_free_register(ctx, arg_reg);
        return false;
    }
    
    /* Generate CALL instruction to printf */
    if (!asm_generate_call(ctx, 0)) {  /* printf address will be resolved later */
        printf("ERROR: Failed to generate CALL instruction\n");
        asm_free_register(ctx, arg_reg);
        return false;
    }
    
    asm_free_register(ctx, arg_reg);
    return true;
}

Bool ast_to_assembly_float_literal(AssemblyContext *ctx, ASTNode *node) {
    if (!ctx || !node || node->type != NODE_FLOAT) return false;
    
    printf("DEBUG: Generating assembly for float literal: %f\n", node->data.literal.f64_value);
    
    /* Generate assembly for float literal (equivalent to HolyC's Print() function) */
    /* This generates a call to printf with the float as argument */
    
    /* Allocate registers for function call */
    X86Register arg_reg = asm_allocate_register(ctx, 8);
    if (arg_reg == REG_NONE) {
        printf("ERROR: Failed to allocate register for float argument\n");
        return false;
    }
    
    /* Setup arguments for printf call */
    CAsmArg format_arg = {0};
    CAsmArg float_arg = {0};
    
    /* Load float value into XMM0 (first floating-point argument register) */
    asm_setup_immediate_arg(ctx, &float_arg, *(I64*)&node->data.literal.f64_value);
    
    /* Generate MOV instruction to load float */
    if (!asm_generate_mov(ctx, &format_arg, &float_arg)) {
        printf("ERROR: Failed to generate MOV instruction for float\n");
        asm_free_register(ctx, arg_reg);
        return false;
    }
    
    /* Generate CALL instruction to printf */
    if (!asm_generate_call(ctx, 0)) {  /* printf address will be resolved later */
        printf("ERROR: Failed to generate CALL instruction\n");
        asm_free_register(ctx, arg_reg);
        return false;
    }
    
    asm_free_register(ctx, arg_reg);
    return true;
}

Bool ast_to_assembly_char_literal(AssemblyContext *ctx, ASTNode *node) {
    if (!ctx || !node || node->type != NODE_CHAR) return false;
    
    printf("DEBUG: Generating assembly for char literal: '%c'\n", node->data.literal.char_value);
    
    /* Generate assembly for char literal (equivalent to HolyC's Print() function) */
    /* This generates a call to printf with the char as argument */
    
    /* Allocate registers for function call */
    X86Register arg_reg = asm_allocate_register(ctx, 8);
    if (arg_reg == REG_NONE) {
        printf("ERROR: Failed to allocate register for char argument\n");
        return false;
    }
    
    /* Setup arguments for printf call */
    CAsmArg format_arg = {0};
    CAsmArg char_arg = {0};
    
    /* Load char value into RCX (first argument register) */
    asm_setup_immediate_arg(ctx, &char_arg, node->data.literal.char_value);
    
    /* Generate MOV instruction to load char */
    if (!asm_generate_mov(ctx, &format_arg, &char_arg)) {
        printf("ERROR: Failed to generate MOV instruction for char\n");
        asm_free_register(ctx, arg_reg);
        return false;
    }
    
    /* Generate CALL instruction to printf */
    if (!asm_generate_call(ctx, 0)) {  /* printf address will be resolved later */
        printf("ERROR: Failed to generate CALL instruction\n");
        asm_free_register(ctx, arg_reg);
        return false;
    }
    
    asm_free_register(ctx, arg_reg);
    return true;
}

/*
 * Placeholder implementations for complex AST nodes
 * These will be implemented as we expand the parser capabilities
 */

Bool ast_to_assembly_binary_operation(AssemblyContext *ctx, ASTNode *node) {
    if (!ctx || !node || node->type != NODE_BINARY_OP) return false;
    
    printf("DEBUG: Generating assembly for binary operation: %d\n", node->data.binary_op.op);
    
    /* Generate assembly for left operand */
    if (!ast_to_assembly_node(ctx, node->data.binary_op.left)) {
        printf("ERROR: Failed to generate assembly for left operand\n");
        return false;
    }
    
    /* Generate assembly for right operand */
    if (!ast_to_assembly_node(ctx, node->data.binary_op.right)) {
        printf("ERROR: Failed to generate assembly for right operand\n");
        return false;
    }
    
    /* Allocate registers for operands */
    X86Register left_reg = asm_allocate_register(ctx, 8);
    X86Register right_reg = asm_allocate_register(ctx, 8);
    X86Register result_reg = asm_allocate_register(ctx, 8);
    
    if (left_reg == REG_NONE || right_reg == REG_NONE || result_reg == REG_NONE) {
        printf("ERROR: Failed to allocate registers for binary operation\n");
        if (left_reg != REG_NONE) asm_free_register(ctx, left_reg);
        if (right_reg != REG_NONE) asm_free_register(ctx, right_reg);
        if (result_reg != REG_NONE) asm_free_register(ctx, result_reg);
        return false;
    }
    
    /* Setup arguments for the operation */
    CAsmArg left_arg = {0};
    CAsmArg right_arg = {0};
    CAsmArg result_arg = {0};
    
    asm_setup_register_arg(ctx, &left_arg, left_reg);
    asm_setup_register_arg(ctx, &right_arg, right_reg);
    asm_setup_register_arg(ctx, &result_arg, result_reg);
    
    /* Generate the appropriate instruction based on operator */
    Bool success = false;
    switch (node->data.binary_op.op) {
        case BINOP_ADD:
            success = asm_generate_add(ctx, &result_arg, &left_arg);
            break;
        case BINOP_SUB:
            success = asm_generate_sub(ctx, &result_arg, &left_arg);
            break;
        case BINOP_MUL:
            success = asm_generate_mul(ctx, &result_arg, &left_arg);
            break;
        case BINOP_DIV:
            success = asm_generate_div(ctx, &result_arg, &left_arg);
            break;
        case BINOP_MOD:
            success = asm_generate_mod(ctx, &result_arg, &left_arg);
            break;
        case BINOP_EQ:
            success = asm_generate_cmp_eq(ctx, &result_arg, &left_arg, &right_arg);
            break;
        case BINOP_NE:
            success = asm_generate_cmp_ne(ctx, &result_arg, &left_arg, &right_arg);
            break;
        case BINOP_LT:
            success = asm_generate_cmp_lt(ctx, &result_arg, &left_arg, &right_arg);
            break;
        case BINOP_LE:
            success = asm_generate_cmp_le(ctx, &result_arg, &left_arg, &right_arg);
            break;
        case BINOP_GT:
            success = asm_generate_cmp_gt(ctx, &result_arg, &left_arg, &right_arg);
            break;
        case BINOP_GE:
            success = asm_generate_cmp_ge(ctx, &result_arg, &left_arg, &right_arg);
            break;
        case BINOP_AND_AND:
            success = asm_generate_logical_and(ctx, &result_arg, &left_arg, &right_arg);
            break;
        case BINOP_OR_OR:
            success = asm_generate_logical_or(ctx, &result_arg, &left_arg, &right_arg);
            break;
        default:
            printf("ERROR: Unsupported binary operator: %d\n", node->data.binary_op.op);
            success = false;
            break;
    }
    
    /* Clean up registers */
    asm_free_register(ctx, left_reg);
    asm_free_register(ctx, right_reg);
    asm_free_register(ctx, result_reg);
    
    if (!success) {
        printf("ERROR: Failed to generate assembly for binary operation\n");
        return false;
    }
    
    printf("DEBUG: Binary operation assembly generated successfully\n");
    return true;
}

Bool ast_to_assembly_unary_operation(AssemblyContext *ctx, ASTNode *node) {
    if (!ctx || !node || node->type != NODE_UNARY_OP) return false;
    
    printf("DEBUG: Generating assembly for unary operation: %d\n", node->data.unary_op.op);
    
    /* Generate assembly for operand */
    if (!ast_to_assembly_node(ctx, node->data.unary_op.operand)) {
        printf("ERROR: Failed to generate assembly for unary operand\n");
        return false;
    }
    
    /* Generate assembly for unary operator */
    switch (node->data.unary_op.op) {
        case UNOP_NOT:
            return asm_generate_logical_not(ctx);
        case UNOP_PLUS:
            return true; /* No operation needed for unary plus */
        case UNOP_MINUS:
            return asm_generate_unary_minus(ctx);
        case UNOP_BITNOT:
            return asm_generate_bitwise_not(ctx);
        case UNOP_INC:
            return asm_generate_pre_increment(ctx);
        case UNOP_DEC:
            return asm_generate_pre_decrement(ctx);
        case UNOP_ADDR:
            return asm_generate_address_of(ctx);
        case UNOP_DEREF:
            return asm_generate_dereference(ctx);
        default:
            printf("ERROR: Unsupported unary operator: %d\n", node->data.unary_op.op);
            return false;
    }
}

Bool ast_to_assembly_function_call(AssemblyContext *ctx, ASTNode *node) {
    if (!ctx || !node || node->type != NODE_CALL) return false;
    
    printf("DEBUG: Generating x64 calling convention for function call: %s\n", 
           node->data.call.name ? (char*)node->data.call.name : "unknown");
    
    /* x86-64 Windows Calling Convention:
     * - First 4 integer arguments: RCX, RDX, R8, R9
     * - Additional arguments: pushed on stack (right to left)
     * - Caller responsible for stack cleanup
     * - Shadow space: 32 bytes must be allocated by caller
     */
    
    I64 arg_count = node->data.call.arg_count;
    I64 stack_args = 0;
    
    /* Allocate shadow space (32 bytes) */
    if (arg_count > 0) {
        /* SUB RSP, 32 - Allocate shadow space */
        if (ctx->instruction_pointer + 4 > ctx->buffer_capacity) {
            printf("ERROR: Not enough space for shadow space allocation\n");
            return false;
        }
        
        ctx->assembly_buffer[ctx->instruction_pointer] = 0x48; /* REX.W prefix */
        ctx->instruction_pointer++;
        ctx->assembly_buffer[ctx->instruction_pointer] = 0x83; /* SUB r/m64, imm8 */
        ctx->instruction_pointer++;
        ctx->assembly_buffer[ctx->instruction_pointer] = 0xEC; /* r/m64 = RSP */
        ctx->instruction_pointer++;
        ctx->assembly_buffer[ctx->instruction_pointer] = 0x20; /* imm8 = 32 bytes */
        ctx->instruction_pointer++;
        
        printf("DEBUG: Allocated 32 bytes shadow space\n");
    }
    
    /* Process function arguments */
    if (node->data.call.arguments && node->data.call.arguments->data.block.statements) {
        ASTNode *arg = node->data.call.arguments->data.block.statements;
        I64 arg_index = 0;
        
        while (arg && arg_index < arg_count) {
            printf("DEBUG: Processing argument %lld\n", arg_index);
            
            /* Generate code to evaluate argument and place in appropriate location */
            if (arg_index < 4) {
                /* First 4 arguments go in registers */
                X86Register reg;
                switch (arg_index) {
                    case 0: reg = REG_RCX; break;
                    case 1: reg = REG_RDX; break;
                    case 2: reg = REG_R8; break;
                    case 3: reg = REG_R9; break;
                    default: reg = REG_RCX; break;
                }
                
                printf("DEBUG: Argument %lld -> register %d\n", arg_index, reg);
                
                /* Generate code to evaluate argument and move to register */
                if (!ast_to_assembly_node(ctx, arg)) {
                    printf("ERROR: Failed to generate assembly for argument %lld\n", arg_index);
                    return false;
                }
                
                /* TODO: Move result to appropriate register */
                /* For now, assume the argument evaluation leaves result in RAX */
                if (reg != REG_RAX) {
                    /* MOV <reg>, RAX */
                    if (ctx->instruction_pointer + 3 > ctx->buffer_capacity) {
                        printf("ERROR: Not enough space for register move\n");
                        return false;
                    }
                    
                    ctx->assembly_buffer[ctx->instruction_pointer] = 0x48; /* REX.W prefix */
                    ctx->instruction_pointer++;
                    ctx->assembly_buffer[ctx->instruction_pointer] = 0x89; /* MOV r/m64, r64 */
                    ctx->instruction_pointer++;
                    
                    /* ModR/M byte: MOV <reg>, RAX */
                    U8 modrm = 0xC0 | (reg << 3) | REG_RAX;
                    ctx->assembly_buffer[ctx->instruction_pointer] = modrm;
                    ctx->instruction_pointer++;
                    
                    printf("DEBUG: Moved argument to register %d\n", reg);
                }
            } else {
                /* Additional arguments go on stack */
                stack_args++;
                
                /* Generate code to evaluate argument */
                if (!ast_to_assembly_node(ctx, arg)) {
                    printf("ERROR: Failed to generate assembly for stack argument %lld\n", arg_index);
                    return false;
                }
                
                /* Push argument onto stack */
                if (ctx->instruction_pointer + 2 > ctx->buffer_capacity) {
                    printf("ERROR: Not enough space for stack push\n");
                    return false;
                }
                
                ctx->assembly_buffer[ctx->instruction_pointer] = 0x50; /* PUSH RAX */
                ctx->instruction_pointer++;
                
                printf("DEBUG: Pushed argument %lld onto stack\n", arg_index);
            }
            
            arg = arg->next;
            arg_index++;
        }
    }
    
    /* Generate CALL instruction */
    I64 call_instruction_size = 5; /* E8 + 32-bit address */
    
    if (ctx->instruction_pointer + call_instruction_size > ctx->buffer_capacity) {
        printf("ERROR: Not enough space in assembly buffer for function call\n");
        return false;
    }
    
    /* Generate the CALL instruction */
    ctx->assembly_buffer[ctx->instruction_pointer] = 0xE8; /* CALL opcode */
    ctx->instruction_pointer++;
    
    /* Implement proper symbol resolution with address calculation */
    if (ctx->parser && node->data.call.name) {
        /* Calculate the function address */
        I64 function_address = parser_calculate_function_address(ctx->parser, node->data.call.name);
        
        if (function_address > 0) {
            /* Calculate relative address for CALL instruction */
            I64 call_address = ctx->instruction_pointer;
            I64 relative_address = parser_calculate_relative_address(ctx->parser, call_address, function_address);
            
            /* Store the relative address in the CALL instruction */
            *(I32*)(&ctx->assembly_buffer[ctx->instruction_pointer]) = (I32)relative_address;
            ctx->instruction_pointer += 4;
            
            printf("DEBUG: Function call '%s' -> address 0x%lx, relative %ld (0x%lx)\n",
                   (char*)node->data.call.name, function_address, relative_address, relative_address);
        } else {
            printf("ERROR: Could not calculate address for function '%s'\n", (char*)node->data.call.name);
            /* Use placeholder address */
            *(I32*)(&ctx->assembly_buffer[ctx->instruction_pointer]) = 0x00000000;
            ctx->instruction_pointer += 4;
        }
    } else {
        /* Fallback to placeholder address */
        *(I32*)(&ctx->assembly_buffer[ctx->instruction_pointer]) = 0x00000000;
        ctx->instruction_pointer += 4;
    }
    
    /* Generate stack cleanup */
    if (stack_args > 0) {
        /* ADD RSP, <stack_args * 8> - Clean up stack arguments */
        I64 cleanup_size = stack_args * 8;
        
        if (ctx->instruction_pointer + 4 > ctx->buffer_capacity) {
            printf("ERROR: Not enough space for stack cleanup\n");
            return false;
        }
        
        ctx->assembly_buffer[ctx->instruction_pointer] = 0x48; /* REX.W prefix */
        ctx->instruction_pointer++;
        ctx->assembly_buffer[ctx->instruction_pointer] = 0x83; /* ADD r/m64, imm8 */
        ctx->instruction_pointer++;
        ctx->assembly_buffer[ctx->instruction_pointer] = 0xC4; /* r/m64 = RSP */
        ctx->instruction_pointer++;
        ctx->assembly_buffer[ctx->instruction_pointer] = (U8)cleanup_size; /* imm8 = cleanup size */
        ctx->instruction_pointer++;
        
        printf("DEBUG: Cleaned up %lld stack arguments (%lld bytes)\n", stack_args, cleanup_size);
    }
    
    /* Clean up shadow space */
    if (arg_count > 0) {
        /* ADD RSP, 32 - Restore shadow space */
        if (ctx->instruction_pointer + 4 > ctx->buffer_capacity) {
            printf("ERROR: Not enough space for shadow space cleanup\n");
            return false;
        }
        
        ctx->assembly_buffer[ctx->instruction_pointer] = 0x48; /* REX.W prefix */
        ctx->instruction_pointer++;
        ctx->assembly_buffer[ctx->instruction_pointer] = 0x83; /* ADD r/m64, imm8 */
        ctx->instruction_pointer++;
        ctx->assembly_buffer[ctx->instruction_pointer] = 0xC4; /* r/m64 = RSP */
        ctx->instruction_pointer++;
        ctx->assembly_buffer[ctx->instruction_pointer] = 0x20; /* imm8 = 32 bytes */
        ctx->instruction_pointer++;
        
        printf("DEBUG: Restored shadow space\n");
    }
    
    printf("DEBUG: Generated x64 calling convention for function call to '%s' (%lld args, %lld stack)\n", 
           node->data.call.name ? (char*)node->data.call.name : "unknown", arg_count, stack_args);
    
    return true;
}

Bool ast_to_assembly_assignment(AssemblyContext *ctx, ASTNode *node) {
    if (!ctx || !node || node->type != NODE_ASSIGNMENT) return false;
    
    printf("DEBUG: Generating assembly for assignment\n");
    
    /* Generate assembly for the right-hand side (value to assign) */
    if (!ast_to_assembly_node(ctx, node->data.assignment.right)) {
        printf("ERROR: Failed to generate assembly for assignment value\n");
        return false;
    }
    
    /* Generate assembly for the left-hand side (variable to assign to) */
    ASTNode *left = node->data.assignment.left;
    if (!left || left->type != NODE_VARIABLE) {
        printf("ERROR: Assignment left-hand side is not a variable\n");
        return false;
    }
    
    /* Allocate a register for the result */
    X86Register result_reg = asm_allocate_register(ctx, 8);
    if (result_reg == REG_NONE) {
        printf("ERROR: Failed to allocate register for assignment result\n");
        return false;
    }
    
    /* Setup arguments for MOV instruction */
    CAsmArg dst_arg = {0};
    CAsmArg src_arg = {0};
    
    /* Setup destination: variable on stack */
    asm_setup_memory_arg(ctx, &dst_arg, REG_RSP, left->data.identifier.stack_offset);
    
    /* Setup source: register with the value */
    asm_setup_register_arg(ctx, &src_arg, result_reg);
    
    /* Generate MOV instruction to store value in variable */
    if (!asm_generate_mov(ctx, &dst_arg, &src_arg)) {
        printf("ERROR: Failed to generate MOV instruction for assignment\n");
        asm_free_register(ctx, result_reg);
        return false;
    }
    
    asm_free_register(ctx, result_reg);
    
    printf("DEBUG: Assignment assembly generated successfully\n");
    return true;
}

Bool ast_to_assembly_variable_declaration(AssemblyContext *ctx, ASTNode *node) {
    if (!ctx || !node || node->type != NODE_VARIABLE) return false;
    
    printf("DEBUG: Generating assembly for variable declaration: %s\n", 
           node->data.identifier.name ? (char*)node->data.identifier.name : "unnamed");
    
    /* Allocate space for variable on stack */
    I64 stack_offset = ctx->stack_offset;
    ctx->stack_offset += 8;  /* 64-bit alignment for all variables */
    
    /* Set the variable's stack offset */
    node->data.identifier.stack_offset = stack_offset;
    
    /* Generate assembly to initialize variable (for now, just allocate space) */
    /* In a real implementation, we might initialize with default values */
    
    printf("DEBUG: Variable %s allocated at stack offset %lld\n", 
           node->data.identifier.name ? (char*)node->data.identifier.name : "unnamed", 
           stack_offset);
    
    return true;
}

Bool ast_to_assembly_variable_reference(AssemblyContext *ctx, ASTNode *node) {
    if (!ctx || !node || node->type != NODE_VARIABLE) return false;
    
    printf("DEBUG: Generating assembly for variable reference: %s\n", 
           node->data.identifier.name ? (char*)node->data.identifier.name : "unnamed");
    
    // Check if the variable has been allocated (has a stack offset)
    if (node->data.identifier.stack_offset < 0) {
        printf("ERROR: Variable %s has not been allocated (stack_offset=%lld)\n", 
               node->data.identifier.name ? (char*)node->data.identifier.name : "unnamed",
               node->data.identifier.stack_offset);
        return false;
    }
    
    // Allocate a register for the variable value
    X86Register result_reg = asm_allocate_register(ctx, 8);
    if (result_reg == REG_NONE) {
        printf("ERROR: Failed to allocate register for variable reference\n");
        return false;
    }
    
    // Generate MOV instruction to load the variable value into the register
    CAsmArg dst_arg = {0};
    CAsmArg src_arg = {0};
    
    asm_setup_register_arg(ctx, &dst_arg, result_reg);
    asm_setup_memory_arg(ctx, &src_arg, REG_RSP, node->data.identifier.stack_offset);
    
    if (!asm_generate_mov(ctx, &dst_arg, &src_arg)) {
        printf("ERROR: Failed to generate MOV instruction for variable reference\n");
        asm_free_register(ctx, result_reg);
        return false;
    }
    
    // Store the result register in the node for use by parent operations
    node->data.identifier.register_id = result_reg;
    
    printf("DEBUG: Variable reference assembly generated successfully, result in register %d\n", result_reg);
    return true;
}

Bool ast_to_assembly_function_declaration(AssemblyContext *ctx, ASTNode *node) {
    if (!ctx || !node || node->type != NODE_FUNCTION) return false;
    
    /* Generate assembly for function declaration */
    
    /* Generate function prologue */
    /* 1. Push RBP (save caller's frame pointer) */
    /* 2. MOV RBP, RSP (set up new frame pointer) */
    /* 3. SUB RSP, <local_variable_space> (allocate space for local variables) */
    
    /* Function prologue: */
    /* 55                   push rbp */
    /* 48 89 E5             mov rbp, rsp */
    /* 48 83 EC <size>      sub rsp, <size> */
    
    I64 prologue_size = 1 + 3 + 4; /* 8 bytes total (push + mov + sub) */
    
    /* Check if we have enough space in the buffer */
    if (ctx->instruction_pointer + prologue_size > ctx->buffer_capacity) {
        printf("ERROR: Not enough space in assembly buffer for function prologue\n");
        return false;
    }
    
    /* Generate function prologue */
    /* Push RBP */
    ctx->assembly_buffer[ctx->instruction_pointer] = 0x55; /* PUSH RBP */
    ctx->instruction_pointer++;
    
    /* MOV RBP, RSP */
    ctx->assembly_buffer[ctx->instruction_pointer] = 0x48; /* REX.W prefix */
    ctx->instruction_pointer++;
    ctx->assembly_buffer[ctx->instruction_pointer] = 0x89; /* MOV opcode */
    ctx->instruction_pointer++;
    ctx->assembly_buffer[ctx->instruction_pointer] = 0xE5; /* RBP, RSP */
    ctx->instruction_pointer++;
    
    /* SUB RSP, <parameter_space> (allocate space for parameters) */
    /* In x86-64 calling convention, parameters are passed in registers and stack */
    /* We need to allocate space for shadow space (32 bytes) + any stack parameters */
    
    /* For now, allocate 32 bytes for shadow space (Windows x64 calling convention) */
    I64 shadow_space = 32; /* 32 bytes of shadow space */
    
    ctx->assembly_buffer[ctx->instruction_pointer] = 0x48; /* REX.W prefix */
    ctx->instruction_pointer++;
    ctx->assembly_buffer[ctx->instruction_pointer] = 0x83; /* SUB opcode */
    ctx->instruction_pointer++;
    ctx->assembly_buffer[ctx->instruction_pointer] = 0xEC; /* RSP register */
    ctx->instruction_pointer++;
    ctx->assembly_buffer[ctx->instruction_pointer] = shadow_space; /* Shadow space size */
    ctx->instruction_pointer++;
    
    /* Generated function prologue (8 bytes) */
    
    /* Generate function body assembly */
    if (node->data.function.body) {
        if (!ast_to_assembly_node(ctx, node->data.function.body)) {
            printf("ERROR: Failed to generate function body assembly\n");
            return false;
        }
    }
    
    /* Generate function epilogue */
    /* 1. MOV RSP, RBP (restore stack pointer) */
    /* 2. POP RBP (restore caller's frame pointer) */
    /* 3. RET (return to caller) */
    
    /* Function epilogue: */
    /* 48 89 EC             mov rsp, rbp */
    /* 5D                   pop rbp */
    /* C3                   ret */
    
    I64 epilogue_size = 3 + 1 + 1; /* 5 bytes total */
    
    /* Check if we have enough space in the buffer */
    if (ctx->instruction_pointer + epilogue_size > ctx->buffer_capacity) {
        printf("ERROR: Not enough space in assembly buffer for function epilogue\n");
        return false;
    }
    
    /* Generate function epilogue */
    /* MOV RSP, RBP */
    ctx->assembly_buffer[ctx->instruction_pointer] = 0x48; /* REX.W prefix */
    ctx->instruction_pointer++;
    ctx->assembly_buffer[ctx->instruction_pointer] = 0x89; /* MOV opcode */
    ctx->instruction_pointer++;
    ctx->assembly_buffer[ctx->instruction_pointer] = 0xEC; /* RSP, RBP */
    ctx->instruction_pointer++;
    
    /* POP RBP */
    ctx->assembly_buffer[ctx->instruction_pointer] = 0x5D; /* POP RBP */
    ctx->instruction_pointer++;
    
    /* RET */
    ctx->assembly_buffer[ctx->instruction_pointer] = 0xC3; /* RET */
    ctx->instruction_pointer++;
    
    /* Generated function epilogue (5 bytes) */
    
    return true;
}

Bool ast_to_assembly_if_statement(AssemblyContext *ctx, ASTNode *node) {
    if (!ctx || !node || node->type != NODE_IF_STMT) return false;
    
    printf("DEBUG: Generating assembly for if statement\n");
    
    /* Generate assembly for if statement:
     * 1. Evaluate condition expression
     * 2. Generate conditional jump to else branch (if exists) or end
     * 3. Generate then branch
     * 4. Generate unconditional jump to end (if else exists)
     * 5. Generate else branch (if exists)
     * 6. Generate end label
     */
    
    /* Step 1: Evaluate condition expression */
    if (node->data.if_stmt.condition) {
        if (!ast_to_assembly_node(ctx, node->data.if_stmt.condition)) {
            printf("ERROR: Failed to generate assembly for if condition\n");
            return false;
        }
    } else {
        printf("ERROR: If statement missing condition\n");
        return false;
    }
    
    /* Step 2: Generate conditional jump
     * For now, we'll assume the condition evaluates to a boolean in RAX
     * JZ (Jump if Zero) - jump to else branch if condition is false (0)
     * JE (Jump if Equal) - same as JZ
     */
    
    /* Reserve space for conditional jump instruction */
    I64 jump_instruction_size = 6; /* 0F 84 <32-bit relative address> (JZ rel32) */
    
    if (ctx->instruction_pointer + jump_instruction_size > ctx->buffer_capacity) {
        printf("ERROR: Not enough space for conditional jump instruction\n");
        return false;
    }
    
    /* Generate conditional jump instruction */
    ctx->assembly_buffer[ctx->instruction_pointer] = 0x0F; /* Two-byte instruction prefix */
    ctx->instruction_pointer++;
    ctx->assembly_buffer[ctx->instruction_pointer] = 0x84; /* JZ (Jump if Zero) opcode */
    ctx->instruction_pointer++;
    
    /* Store placeholder for jump address (will be filled later) */
    I64 jump_address_pos = ctx->instruction_pointer;
    *(I32*)(&ctx->assembly_buffer[ctx->instruction_pointer]) = 0x00000000;
    ctx->instruction_pointer += 4;
    
    /* Step 3: Generate then branch */
    if (node->data.if_stmt.then_stmt) {
        if (!ast_to_assembly_node(ctx, node->data.if_stmt.then_stmt)) {
            printf("ERROR: Failed to generate assembly for if then branch\n");
            return false;
        }
    }
    
    /* Step 4: Generate unconditional jump to end (if else exists) */
    I64 end_jump_pos = -1;
    if (node->data.if_stmt.else_stmt) {
        /* Reserve space for unconditional jump instruction */
        I64 jmp_instruction_size = 5; /* E9 <32-bit relative address> (JMP rel32) */
        
        if (ctx->instruction_pointer + jmp_instruction_size > ctx->buffer_capacity) {
            printf("ERROR: Not enough space for unconditional jump instruction\n");
            return false;
        }
        
        /* Generate unconditional jump instruction */
        ctx->assembly_buffer[ctx->instruction_pointer] = 0xE9; /* JMP opcode */
        ctx->instruction_pointer++;
        
        /* Store placeholder for jump address */
        end_jump_pos = ctx->instruction_pointer;
        *(I32*)(&ctx->assembly_buffer[ctx->instruction_pointer]) = 0x00000000;
        ctx->instruction_pointer += 4;
    }
    
    /* Step 5: Generate else branch (if exists) */
    I64 else_start_pos = ctx->instruction_pointer;
    if (node->data.if_stmt.else_stmt) {
        if (!ast_to_assembly_node(ctx, node->data.if_stmt.else_stmt)) {
            printf("ERROR: Failed to generate assembly for if else branch\n");
            return false;
        }
    }
    
    /* Step 6: Fix up jump addresses */
    I64 end_pos = ctx->instruction_pointer;
    
    /* Fix conditional jump address (jump to else branch or end) */
    I64 conditional_jump_target = node->data.if_stmt.else_stmt ? else_start_pos : end_pos;
    I64 conditional_jump_offset = conditional_jump_target - (jump_address_pos + 4);
    *(I32*)(&ctx->assembly_buffer[jump_address_pos]) = (I32)conditional_jump_offset;
    
    /* Fix unconditional jump address (jump to end) */
    if (end_jump_pos >= 0) {
        I64 end_jump_offset = end_pos - (end_jump_pos + 4);
        *(I32*)(&ctx->assembly_buffer[end_jump_pos]) = (I32)end_jump_offset;
    }
    
    printf("DEBUG: If statement assembly generated successfully\n");
    return true;
}

Bool ast_to_assembly_while_statement(AssemblyContext *ctx, ASTNode *node) {
    if (!ctx || !node || node->type != NODE_WHILE_STMT) return false;
    
    printf("DEBUG: Generating assembly for while statement\n");
    
    /* Generate assembly for while statement:
     * 1. Generate loop start label
     * 2. Evaluate condition expression
     * 3. Generate conditional jump to loop end (if condition is false)
     * 4. Generate loop body
     * 5. Generate unconditional jump back to loop start
     * 6. Generate loop end label
     */
    
    /* Step 1: Remember loop start position */
    I64 loop_start_pos = ctx->instruction_pointer;
    
    /* Step 2: Evaluate condition expression */
    if (node->data.while_stmt.condition) {
        if (!ast_to_assembly_node(ctx, node->data.while_stmt.condition)) {
            printf("ERROR: Failed to generate assembly for while condition\n");
            return false;
        }
    } else {
        printf("ERROR: While statement missing condition\n");
        return false;
    }
    
    /* Step 3: Generate conditional jump to loop end
     * JZ (Jump if Zero) - jump to end if condition is false (0)
     */
    
    /* Reserve space for conditional jump instruction */
    I64 jump_instruction_size = 6; /* 0F 84 <32-bit relative address> (JZ rel32) */
    
    if (ctx->instruction_pointer + jump_instruction_size > ctx->buffer_capacity) {
        printf("ERROR: Not enough space for conditional jump instruction\n");
        return false;
    }
    
    /* Generate conditional jump instruction */
    ctx->assembly_buffer[ctx->instruction_pointer] = 0x0F; /* Two-byte instruction prefix */
    ctx->instruction_pointer++;
    ctx->assembly_buffer[ctx->instruction_pointer] = 0x84; /* JZ (Jump if Zero) opcode */
    ctx->instruction_pointer++;
    
    /* Store placeholder for jump address (will be filled later) */
    I64 jump_address_pos = ctx->instruction_pointer;
    *(I32*)(&ctx->assembly_buffer[ctx->instruction_pointer]) = 0x00000000;
    ctx->instruction_pointer += 4;
    
    /* Step 4: Generate loop body */
    if (node->data.while_stmt.body_stmt) {
        if (!ast_to_assembly_node(ctx, node->data.while_stmt.body_stmt)) {
            printf("ERROR: Failed to generate assembly for while body\n");
            return false;
        }
    }
    
    /* Step 5: Generate unconditional jump back to loop start */
    /* Reserve space for unconditional jump instruction */
    I64 jmp_instruction_size = 5; /* E9 <32-bit relative address> (JMP rel32) */
    
    if (ctx->instruction_pointer + jmp_instruction_size > ctx->buffer_capacity) {
        printf("ERROR: Not enough space for unconditional jump instruction\n");
        return false;
    }
    
    /* Generate unconditional jump instruction */
    ctx->assembly_buffer[ctx->instruction_pointer] = 0xE9; /* JMP opcode */
    ctx->instruction_pointer++;
    
    /* Calculate backward jump to loop start */
    I64 current_pos = ctx->instruction_pointer;
    I64 backward_jump_offset = loop_start_pos - (current_pos + 4);
    *(I32*)(&ctx->assembly_buffer[ctx->instruction_pointer]) = (I32)backward_jump_offset;
    ctx->instruction_pointer += 4;
    
    /* Step 6: Fix up conditional jump address to point to loop end */
    I64 loop_end_pos = ctx->instruction_pointer;
    I64 conditional_jump_offset = loop_end_pos - (jump_address_pos + 4);
    *(I32*)(&ctx->assembly_buffer[jump_address_pos]) = (I32)conditional_jump_offset;
    
    printf("DEBUG: While statement assembly generated successfully\n");
    return true;
}

Bool ast_to_assembly_for_statement(AssemblyContext *ctx, ASTNode *node) {
    if (!ctx || !node || node->type != NODE_FOR_STMT) return false;
    
    printf("DEBUG: Generating assembly for for statement\n");
    
    /* Generate assembly for for statement:
     * 1. Generate initialization code (if exists)
     * 2. Generate loop start label
     * 3. Evaluate condition expression (if exists)
     * 4. Generate conditional jump to loop end (if condition is false)
     * 5. Generate loop body
     * 6. Generate increment code (if exists)
     * 7. Generate unconditional jump back to loop start
     * 8. Generate loop end label
     */
    
    /* For now, for statements are not fully implemented in the parser,
     * so this is a placeholder implementation */
    
    /* TODO: Implement proper for loop assembly generation when parser supports it */
    /* For now, treat it similar to a while loop */
    
    if (node->data.control.condition) {
        /* Generate loop start position */
        I64 loop_start_pos = ctx->instruction_pointer;
        
        /* Evaluate condition */
        if (!ast_to_assembly_node(ctx, node->data.control.condition)) {
            printf("ERROR: Failed to generate assembly for for condition\n");
            return false;
        }
        
        /* Generate conditional jump to end */
        I64 jump_instruction_size = 6; /* 0F 84 <32-bit relative address> (JZ rel32) */
        
        if (ctx->instruction_pointer + jump_instruction_size > ctx->buffer_capacity) {
            printf("ERROR: Not enough space for conditional jump instruction\n");
            return false;
        }
        
        ctx->assembly_buffer[ctx->instruction_pointer] = 0x0F;
        ctx->instruction_pointer++;
        ctx->assembly_buffer[ctx->instruction_pointer] = 0x84;
        ctx->instruction_pointer++;
        
        I64 jump_address_pos = ctx->instruction_pointer;
        *(I32*)(&ctx->assembly_buffer[ctx->instruction_pointer]) = 0x00000000;
        ctx->instruction_pointer += 4;
        
        /* Generate body */
        if (node->data.control.then_body) {
            if (!ast_to_assembly_node(ctx, node->data.control.then_body)) {
                printf("ERROR: Failed to generate assembly for for body\n");
                return false;
            }
        }
        
        /* Generate jump back to start */
        I64 jmp_instruction_size = 5; /* E9 <32-bit relative address> (JMP rel32) */
        
        if (ctx->instruction_pointer + jmp_instruction_size > ctx->buffer_capacity) {
            printf("ERROR: Not enough space for unconditional jump instruction\n");
            return false;
        }
        
        ctx->assembly_buffer[ctx->instruction_pointer] = 0xE9;
        ctx->instruction_pointer++;
        
        I64 current_pos = ctx->instruction_pointer;
        I64 backward_jump_offset = loop_start_pos - (current_pos + 4);
        *(I32*)(&ctx->assembly_buffer[ctx->instruction_pointer]) = (I32)backward_jump_offset;
        ctx->instruction_pointer += 4;
        
        /* Fix up conditional jump address */
        I64 loop_end_pos = ctx->instruction_pointer;
        I64 conditional_jump_offset = loop_end_pos - (jump_address_pos + 4);
        *(I32*)(&ctx->assembly_buffer[jump_address_pos]) = (I32)conditional_jump_offset;
    }
    
    printf("DEBUG: For statement assembly generated successfully (placeholder)\n");
    return true;
}

Bool ast_to_assembly_return_statement(AssemblyContext *ctx, ASTNode *node) {
    if (!ctx || !node || node->type != NODE_RETURN) return false;
    
    printf("DEBUG: Generating x64 return statement assembly\n");
    
    /* In x86-64 calling convention:
     * - Return values are passed in RAX (for integers) or XMM0 (for floats)
     * - Return statement generates: MOV RAX, <value> (if returning a value)
     * - Then: RET (which is already generated in function epilogue)
     */
    
    /* Check if we have a return value or expression */
    if (node->data.return_stmt.return_value != 0) {
        /* Simple return value (integer literal) */
        I64 return_value = node->data.return_stmt.return_value;
        
        printf("DEBUG: Returning simple value: %lld\n", return_value);
        
        /* Generate: MOV RAX, <return_value> */
        /* MOV RAX, immediate: 48 C7 C0 <32-bit value> */
        
        I64 mov_instruction_size = 7; /* REX.W + MOV opcode + MODRM + 32-bit immediate */
        
        /* Check if we have enough space in the buffer */
        if (ctx->instruction_pointer + mov_instruction_size > ctx->buffer_capacity) {
            printf("ERROR: Not enough space in assembly buffer for return statement\n");
            return false;
        }
        
        /* Generate MOV RAX, <return_value> */
        ctx->assembly_buffer[ctx->instruction_pointer] = 0x48; /* REX.W prefix */
        ctx->instruction_pointer++;
        ctx->assembly_buffer[ctx->instruction_pointer] = 0xC7; /* MOV opcode */
        ctx->instruction_pointer++;
        ctx->assembly_buffer[ctx->instruction_pointer] = 0xC0; /* RAX register */
        ctx->instruction_pointer++;
        
        /* Store 32-bit immediate value */
        *(I32*)(&ctx->assembly_buffer[ctx->instruction_pointer]) = (I32)return_value;
        ctx->instruction_pointer += 4;
        
        printf("DEBUG: Generated MOV RAX, %lld\n", return_value);
        
    } else if (node->data.return_stmt.expression) {
        /* Complex return expression */
        printf("DEBUG: Returning complex expression\n");
        
        /* Generate assembly for the return expression */
        if (!ast_to_assembly_node(ctx, node->data.return_stmt.expression)) {
            printf("ERROR: Failed to generate assembly for return expression\n");
            return false;
        }
        
        /* The expression evaluation should leave the result in RAX */
        /* If it doesn't, we need to move it there */
        printf("DEBUG: Return expression evaluated, result should be in RAX\n");
        
    } else {
        /* No return value - just return */
        printf("DEBUG: Returning void (no value)\n");
    }
    
    /* Note: The actual RET instruction is generated in the function epilogue */
    /* So we don't generate it here to avoid duplication */
    
    printf("DEBUG: Return statement assembly generated successfully\n");
    return true;
}

Bool ast_to_assembly_block_statement(AssemblyContext *ctx, ASTNode *node) {
    if (!ctx || !node || node->type != NODE_BLOCK) return false;
    
    printf("DEBUG: Generating assembly for block statement\n");
    
    /* Generate assembly for block statement:
     * A block statement contains a list of statements that should be executed sequentially
     * 1. Process each statement in the block sequentially
     * 2. No special assembly is needed for the block itself - just process children
     */
    
    /* Process each statement in the block */
    ASTNode *statement = node->data.block.statements;
    I64 statement_count = 0;
    
    while (statement) {
        printf("DEBUG: Processing block statement %lld, type %d\n", statement_count, statement->type);
        
        if (!ast_to_assembly_node(ctx, statement)) {
            printf("ERROR: Failed to generate assembly for block statement %lld\n", statement_count);
            return false;
        }
        
        statement_count++;
        statement = statement->next;
    }
    
    printf("DEBUG: Block statement assembly generated successfully (%lld statements)\n", statement_count);
    return true;
}

Bool ast_to_assembly_inline_assembly(AssemblyContext *ctx, ASTNode *node) {
    printf("DEBUG: Inline assembly generation not yet implemented\n");
    return true;  /* Placeholder - don't fail */
}

/*
 * Relational and Logical Operation Assembly Generation
 */

Bool asm_generate_cmp_eq(AssemblyContext *ctx, CAsmArg *result, CAsmArg *left, CAsmArg *right) {
    if (!ctx || !result || !left || !right) return false;
    
    printf("DEBUG: Generating assembly for equality comparison (==)\n");
    
    /* Generate CMP instruction to compare left and right operands */
    /* CMP left, right */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x48; /* REX.W prefix for 64-bit */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x39; /* CMP r/m64, r64 */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xC0; /* ModR/M: reg=right, r/m=left */
    
    /* Generate SETE instruction to set result based on zero flag */
    /* SETE result */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x0F; /* Two-byte opcode prefix */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x94; /* SETE */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xC0; /* ModR/M: reg=result */
    
    return true;
}

Bool asm_generate_cmp_ne(AssemblyContext *ctx, CAsmArg *result, CAsmArg *left, CAsmArg *right) {
    if (!ctx || !result || !left || !right) return false;
    
    printf("DEBUG: Generating assembly for inequality comparison (!=)\n");
    
    /* Generate CMP instruction to compare left and right operands */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x48; /* REX.W prefix for 64-bit */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x39; /* CMP r/m64, r64 */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xC0; /* ModR/M: reg=right, r/m=left */
    
    /* Generate SETNE instruction to set result based on zero flag */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x0F; /* Two-byte opcode prefix */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x95; /* SETNE */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xC0; /* ModR/M: reg=result */
    
    return true;
}

Bool asm_generate_cmp_lt(AssemblyContext *ctx, CAsmArg *result, CAsmArg *left, CAsmArg *right) {
    if (!ctx || !result || !left || !right) return false;
    
    printf("DEBUG: Generating assembly for less-than comparison (<)\n");
    
    /* Generate CMP instruction to compare left and right operands */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x48; /* REX.W prefix for 64-bit */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x39; /* CMP r/m64, r64 */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xC0; /* ModR/M: reg=right, r/m=left */
    
    /* Generate SETL instruction to set result based on sign flag */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x0F; /* Two-byte opcode prefix */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x9C; /* SETL */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xC0; /* ModR/M: reg=result */
    
    return true;
}

Bool asm_generate_cmp_le(AssemblyContext *ctx, CAsmArg *result, CAsmArg *left, CAsmArg *right) {
    if (!ctx || !result || !left || !right) return false;
    
    printf("DEBUG: Generating assembly for less-than-or-equal comparison (<=)\n");
    
    /* Generate CMP instruction to compare left and right operands */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x48; /* REX.W prefix for 64-bit */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x39; /* CMP r/m64, r64 */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xC0; /* ModR/M: reg=right, r/m=left */
    
    /* Generate SETLE instruction to set result based on flags */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x0F; /* Two-byte opcode prefix */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x9E; /* SETLE */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xC0; /* ModR/M: reg=result */
    
    return true;
}

Bool asm_generate_cmp_gt(AssemblyContext *ctx, CAsmArg *result, CAsmArg *left, CAsmArg *right) {
    if (!ctx || !result || !left || !right) return false;
    
    printf("DEBUG: Generating assembly for greater-than comparison (>)\n");
    
    /* Generate CMP instruction to compare left and right operands */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x48; /* REX.W prefix for 64-bit */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x39; /* CMP r/m64, r64 */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xC0; /* ModR/M: reg=right, r/m=left */
    
    /* Generate SETG instruction to set result based on flags */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x0F; /* Two-byte opcode prefix */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x9F; /* SETG */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xC0; /* ModR/M: reg=result */
    
    return true;
}

Bool asm_generate_cmp_ge(AssemblyContext *ctx, CAsmArg *result, CAsmArg *left, CAsmArg *right) {
    if (!ctx || !result || !left || !right) return false;
    
    printf("DEBUG: Generating assembly for greater-than-or-equal comparison (>=)\n");
    
    /* Generate CMP instruction to compare left and right operands */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x48; /* REX.W prefix for 64-bit */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x39; /* CMP r/m64, r64 */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xC0; /* ModR/M: reg=right, r/m=left */
    
    /* Generate SETGE instruction to set result based on flags */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x0F; /* Two-byte opcode prefix */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x9D; /* SETGE */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xC0; /* ModR/M: reg=result */
    
    return true;
}

Bool asm_generate_logical_and(AssemblyContext *ctx, CAsmArg *result, CAsmArg *left, CAsmArg *right) {
    if (!ctx || !result || !left || !right) return false;
    
    printf("DEBUG: Generating assembly for logical AND (&&)\n");
    
    /* For logical AND, we need to:
     * 1. Test left operand (CMP left, 0)
     * 2. If left is false, jump to end with result=0
     * 3. Test right operand (CMP right, 0) 
     * 4. Set result based on right operand
     */
    
    /* Test left operand: CMP left, 0 */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x48; /* REX.W prefix */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x83; /* CMP r/m64, imm8 */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xF8; /* ModR/M: reg=left */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x00; /* Immediate value 0 */
    
    /* Jump if zero (left is false): JZ end */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x74; /* JZ rel8 */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x05; /* Jump 5 bytes ahead */
    
    /* Test right operand: CMP right, 0 */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x48; /* REX.W prefix */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x83; /* CMP r/m64, imm8 */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xF8; /* ModR/M: reg=right */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x00; /* Immediate value 0 */
    
    /* Set result based on zero flag: SETNE result */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x0F; /* Two-byte opcode prefix */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x95; /* SETNE */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xC0; /* ModR/M: reg=result */
    
    /* JMP to end */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xEB; /* JMP rel8 */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x02; /* Jump 2 bytes ahead */
    
    /* End: Set result to 0 */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x31; /* XOR r64, r64 */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xC0; /* ModR/M: reg=result */
    
    return true;
}

Bool asm_generate_logical_or(AssemblyContext *ctx, CAsmArg *result, CAsmArg *left, CAsmArg *right) {
    if (!ctx || !result || !left || !right) return false;
    
    printf("DEBUG: Generating assembly for logical OR (||)\n");
    
    /* For logical OR, we need to:
     * 1. Test left operand (CMP left, 0)
     * 2. If left is true, jump to end with result=1
     * 3. Test right operand (CMP right, 0)
     * 4. Set result based on right operand
     */
    
    /* Test left operand: CMP left, 0 */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x48; /* REX.W prefix */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x83; /* CMP r/m64, imm8 */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xF8; /* ModR/M: reg=left */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x00; /* Immediate value 0 */
    
    /* Jump if not zero (left is true): JNZ end */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x75; /* JNZ rel8 */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x05; /* Jump 5 bytes ahead */
    
    /* Test right operand: CMP right, 0 */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x48; /* REX.W prefix */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x83; /* CMP r/m64, imm8 */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xF8; /* ModR/M: reg=right */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x00; /* Immediate value 0 */
    
    /* Set result based on zero flag: SETNE result */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x0F; /* Two-byte opcode prefix */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x95; /* SETNE */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xC0; /* ModR/M: reg=result */
    
    /* JMP to end */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xEB; /* JMP rel8 */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x02; /* Jump 2 bytes ahead */
    
    /* End: Set result to 1 */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xB0; /* MOV r8, imm8 */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x01; /* Immediate value 1 */
    
    return true;
}

/*
 * Unary Operation Assembly Generation
 */

Bool asm_generate_logical_not(AssemblyContext *ctx) {
    if (!ctx) return false;
    
    printf("DEBUG: Generating assembly for logical NOT (!)\n");
    
    /* For logical NOT, we need to:
     * 1. Test if operand is zero (CMP operand, 0)
     * 2. Set result to 1 if zero (SETE), 0 if non-zero (SETNE)
     */
    
    /* Test operand: CMP operand, 0 */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x48; /* REX.W prefix */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x83; /* CMP r/m64, imm8 */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xF8; /* ModR/M: reg=operand */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x00; /* Immediate value 0 */
    
    /* Set result based on zero flag: SETE result (1 if zero, 0 if non-zero) */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x0F; /* Two-byte opcode prefix */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x94; /* SETE */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xC0; /* ModR/M: reg=result */
    
    return true;
}

Bool asm_generate_unary_minus(AssemblyContext *ctx) {
    if (!ctx) return false;
    
    printf("DEBUG: Generating assembly for unary minus (-)\n");
    
    /* For unary minus, we need to negate the operand:
     * NEG operand
     */
    
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x48; /* REX.W prefix for 64-bit */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xF7; /* NEG r/m64 */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xD8; /* ModR/M: reg=operand */
    
    return true;
}

Bool asm_generate_bitwise_not(AssemblyContext *ctx) {
    if (!ctx) return false;
    
    printf("DEBUG: Generating assembly for bitwise NOT (~)\n");
    
    /* For bitwise NOT, we need to complement all bits:
     * NOT operand
     */
    
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x48; /* REX.W prefix for 64-bit */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xF7; /* NOT r/m64 */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xD0; /* ModR/M: reg=operand */
    
    return true;
}

Bool asm_generate_pre_increment(AssemblyContext *ctx) {
    if (!ctx) return false;
    
    printf("DEBUG: Generating assembly for pre-increment (++x)\n");
    
    /* For pre-increment, we need to:
     * 1. Increment the operand (INC operand)
     * 2. The result is the new value
     */
    
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x48; /* REX.W prefix for 64-bit */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xFF; /* INC r/m64 */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xC0; /* ModR/M: reg=operand */
    
    return true;
}

Bool asm_generate_pre_decrement(AssemblyContext *ctx) {
    if (!ctx) return false;
    
    printf("DEBUG: Generating assembly for pre-decrement (--x)\n");
    
    /* For pre-decrement, we need to:
     * 1. Decrement the operand (DEC operand)
     * 2. The result is the new value
     */
    
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x48; /* REX.W prefix for 64-bit */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xFF; /* DEC r/m64 */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0xC8; /* ModR/M: reg=operand */
    
    return true;
}

Bool asm_generate_address_of(AssemblyContext *ctx) {
    if (!ctx) return false;
    
    printf("DEBUG: Generating assembly for address-of (&x)\n");
    
    /* For address-of, we need to:
     * 1. Load effective address (LEA result, [operand])
     * 2. This loads the address of the operand into result
     */
    
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x48; /* REX.W prefix for 64-bit */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x8D; /* LEA r64, m */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x05; /* ModR/M: result=result, m=[operand] */
    
    /* Note: This is a simplified implementation. In practice, we would need
     * to handle different addressing modes and operand types */
    
    return true;
}

Bool asm_generate_dereference(AssemblyContext *ctx) {
    if (!ctx) return false;
    
    printf("DEBUG: Generating assembly for dereference (*x)\n");
    
    /* For dereference, we need to:
     * 1. Load value from memory (MOV result, [operand])
     * 2. This loads the value at the address stored in operand
     */
    
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x48; /* REX.W prefix for 64-bit */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x8B; /* MOV r64, r/m64 */
    ctx->assembly_buffer[ctx->instruction_pointer++] = 0x00; /* ModR/M: result=result, m=[operand] */
    
    /* Note: This is a simplified implementation. In practice, we would need
     * to handle different addressing modes and operand types */
    
    return true;
}

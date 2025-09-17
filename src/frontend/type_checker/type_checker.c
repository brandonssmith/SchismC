/*
 * Type Checker Implementation for SchismC
 * Provides type validation and coercion for expressions
 */

#include "type_checker.h"
#include "parser.h"
#include "lexer.h"
#include <string.h>
#include <stdio.h>

/* Type compatibility matrix */
typedef struct {
    SchismTokenType from_type;
    SchismTokenType to_type;
    Bool is_compatible;
    Bool requires_coercion;
} TypeCompatibility;

static TypeCompatibility type_compatibility_matrix[] = {
    /* Integer to Integer conversions */
    {TK_TYPE_I8, TK_TYPE_I16, true, true},
    {TK_TYPE_I8, TK_TYPE_I32, true, true},
    {TK_TYPE_I8, TK_TYPE_I64, true, true},
    {TK_TYPE_I16, TK_TYPE_I32, true, true},
    {TK_TYPE_I16, TK_TYPE_I64, true, true},
    {TK_TYPE_I32, TK_TYPE_I64, true, true},
    
    /* Unsigned to Unsigned conversions */
    {TK_TYPE_U8, TK_TYPE_U16, true, true},
    {TK_TYPE_U8, TK_TYPE_U32, true, true},
    {TK_TYPE_U8, TK_TYPE_U64, true, true},
    {TK_TYPE_U16, TK_TYPE_U32, true, true},
    {TK_TYPE_U16, TK_TYPE_U64, true, true},
    {TK_TYPE_U32, TK_TYPE_U64, true, true},
    
    /* Integer to Float conversions */
    {TK_TYPE_I8, TK_TYPE_F32, true, true},
    {TK_TYPE_I8, TK_TYPE_F64, true, true},
    {TK_TYPE_I16, TK_TYPE_F32, true, true},
    {TK_TYPE_I16, TK_TYPE_F64, true, true},
    {TK_TYPE_I32, TK_TYPE_F32, true, true},
    {TK_TYPE_I32, TK_TYPE_F64, true, true},
    {TK_TYPE_I64, TK_TYPE_F64, true, true},
    
    /* Float to Float conversions */
    {TK_TYPE_F32, TK_TYPE_F64, true, true},
    
    /* Boolean conversions */
    {TK_TYPE_BOOL, TK_TYPE_I8, true, true},
    {TK_TYPE_BOOL, TK_TYPE_I16, true, true},
    {TK_TYPE_BOOL, TK_TYPE_I32, true, true},
    {TK_TYPE_BOOL, TK_TYPE_I64, true, true},
    {TK_TYPE_I8, TK_TYPE_BOOL, true, true},
    {TK_TYPE_I16, TK_TYPE_BOOL, true, true},
    {TK_TYPE_I32, TK_TYPE_BOOL, true, true},
    {TK_TYPE_I64, TK_TYPE_BOOL, true, true},
    
    /* Same type (no conversion needed) */
    {TK_TYPE_I8, TK_TYPE_I8, true, false},
    {TK_TYPE_I16, TK_TYPE_I16, true, false},
    {TK_TYPE_I32, TK_TYPE_I32, true, false},
    {TK_TYPE_I64, TK_TYPE_I64, true, false},
    {TK_TYPE_U8, TK_TYPE_U8, true, false},
    {TK_TYPE_U16, TK_TYPE_U16, true, false},
    {TK_TYPE_U32, TK_TYPE_U32, true, false},
    {TK_TYPE_U64, TK_TYPE_U64, true, false},
    {TK_TYPE_F32, TK_TYPE_F32, true, false},
    {TK_TYPE_F64, TK_TYPE_F64, true, false},
    {TK_TYPE_BOOL, TK_TYPE_BOOL, true, false},
    {TK_TYPE_STRING, TK_TYPE_STRING, true, false},
    
    /* End marker */
    {0, 0, false, false}
};

/* Get type name for debugging */
const char* type_get_name(SchismTokenType type) {
    switch (type) {
        case TK_TYPE_I8: return "I8";
        case TK_TYPE_I16: return "I16";
        case TK_TYPE_I32: return "I32";
        case TK_TYPE_I64: return "I64";
        case TK_TYPE_U8: return "U8";
        case TK_TYPE_U16: return "U16";
        case TK_TYPE_U32: return "U32";
        case TK_TYPE_U64: return "U64";
        case TK_TYPE_F32: return "F32";
        case TK_TYPE_F64: return "F64";
        case TK_TYPE_BOOL: return "Bool";
        case TK_TYPE_STRING: return "String";
        default: return "Unknown";
    }
}

/* Check if two types are compatible */
Bool type_is_compatible(SchismTokenType from_type, SchismTokenType to_type) {
    if (from_type == to_type) return true;
    
    for (I64 i = 0; type_compatibility_matrix[i].from_type != 0; i++) {
        if (type_compatibility_matrix[i].from_type == from_type &&
            type_compatibility_matrix[i].to_type == to_type) {
            return type_compatibility_matrix[i].is_compatible;
        }
    }
    
    return false;
}

/* Check if type conversion requires coercion */
Bool type_requires_coercion(SchismTokenType from_type, SchismTokenType to_type) {
    if (from_type == to_type) return false;
    
    for (I64 i = 0; type_compatibility_matrix[i].from_type != 0; i++) {
        if (type_compatibility_matrix[i].from_type == from_type &&
            type_compatibility_matrix[i].to_type == to_type) {
            return type_compatibility_matrix[i].requires_coercion;
        }
    }
    
    return false;
}

/* Get the result type for a binary operation */
SchismTokenType type_get_binary_result_type(SchismTokenType left_type, SchismTokenType right_type, BinaryOpType op) {
    /* Arithmetic operations */
    if (op == BINOP_ADD || op == BINOP_SUB || op == BINOP_MUL || op == BINOP_DIV || op == BINOP_MOD) {
        /* If either operand is float, result is float */
        if (left_type == TK_TYPE_F64 || right_type == TK_TYPE_F64) {
            return TK_TYPE_F64;
        }
        if (left_type == TK_TYPE_F32 || right_type == TK_TYPE_F32) {
            return TK_TYPE_F32;
        }
        
        /* If either operand is unsigned, result is unsigned */
        if ((left_type >= TK_TYPE_U8 && left_type <= TK_TYPE_U64) ||
            (right_type >= TK_TYPE_U8 && right_type <= TK_TYPE_U64)) {
            /* Return the larger unsigned type */
            if (left_type == TK_TYPE_U64 || right_type == TK_TYPE_U64) return TK_TYPE_U64;
            if (left_type == TK_TYPE_U32 || right_type == TK_TYPE_U32) return TK_TYPE_U32;
            if (left_type == TK_TYPE_U16 || right_type == TK_TYPE_U16) return TK_TYPE_U16;
            return TK_TYPE_U8;
        }
        
        /* Both signed integers - return the larger type */
        if (left_type == TK_TYPE_I64 || right_type == TK_TYPE_I64) return TK_TYPE_I64;
        if (left_type == TK_TYPE_I32 || right_type == TK_TYPE_I32) return TK_TYPE_I32;
        if (left_type == TK_TYPE_I16 || right_type == TK_TYPE_I16) return TK_TYPE_I16;
        return TK_TYPE_I8;
    }
    
    /* Comparison operations */
    if (op == BINOP_EQ || op == BINOP_NE || op == BINOP_LT || op == BINOP_LE || 
        op == BINOP_GT || op == BINOP_GE) {
        return TK_TYPE_BOOL;
    }
    
    /* Logical operations */
    if (op == BINOP_AND_AND || op == BINOP_OR_OR) {
        return TK_TYPE_BOOL;
    }
    
    /* Default to left type */
    return left_type;
}

/* Validate assignment compatibility */
Bool type_validate_assignment(SchismTokenType left_type, SchismTokenType right_type) {
    if (type_is_compatible(right_type, left_type)) {
        return true;
    }
    
    printf("TYPE ERROR: Cannot assign %s to %s\n", 
           type_get_name(right_type), type_get_name(left_type));
    return false;
}

/* Validate binary operation compatibility */
Bool type_validate_binary_operation(SchismTokenType left_type, SchismTokenType right_type, BinaryOpType op) {
    /* Logical operations require boolean operands */
    if (op == BINOP_AND_AND || op == BINOP_OR_OR) {
        if (left_type != TK_TYPE_BOOL || right_type != TK_TYPE_BOOL) {
            printf("TYPE ERROR: Logical operation requires boolean operands, got %s and %s\n",
                   type_get_name(left_type), type_get_name(right_type));
            return false;
        }
        return true;
    }
    
    /* Arithmetic operations require numeric operands */
    if (op == BINOP_ADD || op == BINOP_SUB || op == BINOP_MUL || op == BINOP_DIV || op == BINOP_MOD) {
        if ((left_type < TK_TYPE_I8 || left_type > TK_TYPE_F64) ||
            (right_type < TK_TYPE_I8 || right_type > TK_TYPE_F64)) {
            printf("TYPE ERROR: Arithmetic operation requires numeric operands, got %s and %s\n",
                   type_get_name(left_type), type_get_name(right_type));
            return false;
        }
        return true;
    }
    
    /* Comparison operations require compatible types */
    if (op == BINOP_EQ || op == BINOP_NE || op == BINOP_LT || op == BINOP_LE || 
        op == BINOP_GT || op == BINOP_GE) {
        if (!type_is_compatible(left_type, right_type) && !type_is_compatible(right_type, left_type)) {
            printf("TYPE ERROR: Comparison operation requires compatible types, got %s and %s\n",
                   type_get_name(left_type), type_get_name(right_type));
            return false;
        }
        return true;
    }
    
    return true;
}

/* Get the type of an AST node */
SchismTokenType type_get_ast_node_type(ASTNode *node) {
    if (!node) return 0;
    
    switch (node->type) {
        case NODE_INTEGER:
            return TK_TYPE_I64; /* Default integer type */
        case NODE_FLOAT:
            return TK_TYPE_F64; /* Default float type */
        case NODE_STRING:
            return TK_TYPE_STRING;
        case NODE_BOOLEAN:
            return TK_TYPE_BOOL;
        case NODE_IDENTIFIER:
        case NODE_VARIABLE:
            /* Get type from variable declaration */
            if (node->data.identifier.type) {
                return (SchismTokenType)(I64)node->data.identifier.type;
            }
            return TK_TYPE_I64; /* Default */
        case NODE_BINARY_OP:
            /* Get result type from binary operation */
            return type_get_binary_result_type(
                type_get_ast_node_type(node->data.binary_op.left),
                type_get_ast_node_type(node->data.binary_op.right),
                node->data.binary_op.op
            );
        case NODE_UNARY_OP:
            /* Unary operations typically preserve type */
            return type_get_ast_node_type(node->data.unary_op.operand);
        case NODE_CALL:
            /* Function call returns the function's return type */
            if (node->data.call.return_type) {
                return (SchismTokenType)(I64)node->data.call.return_type;
            }
            return TK_TYPE_I64; /* Default */
        case NODE_SUB_INT_ACCESS:
            /* Sub-int access returns the member type */
            {
                U8 *member_type = node->data.sub_int_access.member_type;
                if (member_type) {
                    if (strcmp(member_type, "i8") == 0) return TK_TYPE_I8;
                    if (strcmp(member_type, "u8") == 0) return TK_TYPE_U8;
                    if (strcmp(member_type, "i16") == 0) return TK_TYPE_I16;
                    if (strcmp(member_type, "u16") == 0) return TK_TYPE_U16;
                    if (strcmp(member_type, "i32") == 0) return TK_TYPE_I32;
                    if (strcmp(member_type, "u32") == 0) return TK_TYPE_U32;
                }
            }
            return TK_TYPE_I64; /* Default */
        case NODE_UNION_MEMBER_ACCESS:
            /* Union member access - type depends on the member */
            /* For now, return a default type - this would need to be enhanced
               to look up the actual member type from the union definition */
            return TK_TYPE_I64; /* Default */
        default:
            return TK_TYPE_I64; /* Default type */
    }
}

/* Type check an AST node */
Bool type_check_ast_node(ASTNode *node) {
    if (!node) return true;
    
    switch (node->type) {
        case NODE_ASSIGNMENT:
            /* Check assignment compatibility */
            {
                SchismTokenType left_type = type_get_ast_node_type(node->data.assignment.left);
                SchismTokenType right_type = type_get_ast_node_type(node->data.assignment.right);
                
                if (!type_validate_assignment(left_type, right_type)) {
                    return false;
                }
            }
            break;
            
        case NODE_BINARY_OP:
            /* Check binary operation compatibility */
            {
                SchismTokenType left_type = type_get_ast_node_type(node->data.binary_op.left);
                SchismTokenType right_type = type_get_ast_node_type(node->data.binary_op.right);
                
                if (!type_validate_binary_operation(left_type, right_type, node->data.binary_op.op)) {
                    return false;
                }
            }
            break;
            
        case NODE_VARIABLE:
            /* Check variable initialization */
            if (node->data.variable.initializer) {
                SchismTokenType var_type = (SchismTokenType)(I64)node->data.variable.type;
                SchismTokenType init_type = type_get_ast_node_type(node->data.variable.initializer);
                
                if (!type_validate_assignment(var_type, init_type)) {
                    return false;
                }
            }
            break;
            
        case NODE_SUB_INT_ACCESS:
            /* Check sub-int access */
            {
                /* Validate that the base object exists and is accessible */
                if (!node->data.sub_int_access.base_object) {
                    printf("ERROR: Sub-int access missing base object\n");
                    return false;
                }
                
                /* Validate that the index expression is valid */
                if (!node->data.sub_int_access.index) {
                    printf("ERROR: Sub-int access missing index expression\n");
                    return false;
                }
                
                /* Check that the index is an integer type */
                SchismTokenType index_type = type_get_ast_node_type(node->data.sub_int_access.index);
                if (index_type != TK_TYPE_I8 && index_type != TK_TYPE_I16 && 
                    index_type != TK_TYPE_I32 && index_type != TK_TYPE_I64 &&
                    index_type != TK_TYPE_U8 && index_type != TK_TYPE_U16 && 
                    index_type != TK_TYPE_U32 && index_type != TK_TYPE_U64) {
                    printf("ERROR: Sub-int access index must be an integer type\n");
                    return false;
                }
                
                /* Validate member type */
                U8 *member_type = node->data.sub_int_access.member_type;
                if (!member_type) {
                    printf("ERROR: Sub-int access missing member type\n");
                    return false;
                }
                
                /* Check bounds based on member size */
                I64 member_size = node->data.sub_int_access.member_size;
                if (member_size <= 0) {
                    printf("ERROR: Invalid member size for sub-int access\n");
                    return false;
                }
            }
            break;
            
        case NODE_UNION_MEMBER_ACCESS:
            /* Check union member access */
            {
                /* Validate that the union object exists */
                if (!node->data.union_member_access.union_object) {
                    printf("ERROR: Union member access missing union object\n");
                    return false;
                }
                
                /* Validate that the index expression is valid */
                if (!node->data.union_member_access.index) {
                    printf("ERROR: Union member access missing index expression\n");
                    return false;
                }
                
                /* Check that the index is an integer type */
                SchismTokenType index_type = type_get_ast_node_type(node->data.union_member_access.index);
                if (index_type != TK_TYPE_I8 && index_type != TK_TYPE_I16 && 
                    index_type != TK_TYPE_I32 && index_type != TK_TYPE_I64 &&
                    index_type != TK_TYPE_U8 && index_type != TK_TYPE_U16 && 
                    index_type != TK_TYPE_U32 && index_type != TK_TYPE_U64) {
                    printf("ERROR: Union member access index must be an integer type\n");
                    return false;
                }
            }
            break;
    }
    
    /* Recursively check children */
    ASTNode *child = node->children;
    while (child) {
        if (!type_check_ast_node(child)) {
            return false;
        }
        child = child->next;
    }
    
    return true;
}


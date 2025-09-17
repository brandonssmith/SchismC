/*
 * Type Checker Header for SchismC
 * Provides type validation and coercion for expressions
 */

#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#include "core_structures.h"
#include "parser.h"
#include "lexer.h"

/* Type checking functions */
Bool type_is_compatible(SchismTokenType from_type, SchismTokenType to_type);
Bool type_requires_coercion(SchismTokenType from_type, SchismTokenType to_type);
SchismTokenType type_get_binary_result_type(SchismTokenType left_type, SchismTokenType right_type, BinaryOpType op);
Bool type_validate_assignment(SchismTokenType left_type, SchismTokenType right_type);
Bool type_validate_binary_operation(SchismTokenType left_type, SchismTokenType right_type, BinaryOpType op);
SchismTokenType type_get_ast_node_type(ASTNode *node);
Bool type_check_ast_node(ASTNode *node);

/* Utility functions */
const char* type_get_name(SchismTokenType type);

#endif /* TYPE_CHECKER_H */


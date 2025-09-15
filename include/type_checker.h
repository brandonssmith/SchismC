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
Bool type_is_compatible(TokenType from_type, TokenType to_type);
Bool type_requires_coercion(TokenType from_type, TokenType to_type);
TokenType type_get_binary_result_type(TokenType left_type, TokenType right_type, BinaryOpType op);
Bool type_validate_assignment(TokenType left_type, TokenType right_type);
Bool type_validate_binary_operation(TokenType left_type, TokenType right_type, BinaryOpType op);
TokenType type_get_ast_node_type(ASTNode *node);
Bool type_check_ast_node(ASTNode *node);

/* Utility functions */
const char* type_get_name(TokenType type);

#endif /* TYPE_CHECKER_H */


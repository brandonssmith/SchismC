/*
 * Enhanced Parser Implementation for SchismC
 * Ported from TempleOS HolyC with assembly-influenced enhancements
 */

#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward declarations */
ASTNode* parse_program(ParserState *parser);
ASTNode* parse_statement(ParserState *parser);
ASTNode* parse_assignment_or_expression_statement(ParserState *parser);

/*
 * Parser management functions
 */

ParserState* parser_new(LexerState *lexer, CCmpCtrl *cc) {
    ParserState *parser = (ParserState*)malloc(sizeof(ParserState));
    if (!parser) return NULL;
    
    /* Initialize parser state */
    memset(parser, 0, sizeof(ParserState));
    parser->lexer = lexer;
    parser->cc = cc;
    parser->error_count = 0;
    parser->warning_count = 0;
    parser->last_error = NULL;
    
    /* Initialize symbol table */
    parser->symbol_table.capacity = 256;
    parser->symbol_table.symbols = (ASTNode**)calloc(parser->symbol_table.capacity, sizeof(ASTNode*));
    if (!parser->symbol_table.symbols) {
        free(parser);
        return NULL;
    }
    
    /* Initialize address space */
    parser_initialize_address_space(parser);
    
    /* Initialize scope stack */
    parser->scope_stack.scope_count = 0;
    parser->scope_stack.scope_capacity = 16;
    parser->scope_stack.current_scope_depth = 0;
    parser->scope_stack.scopes = (ScopeLevel**)calloc(parser->scope_stack.scope_capacity, sizeof(ScopeLevel*));
    if (!parser->scope_stack.scopes) {
        free(parser->symbol_table.symbols);
        free(parser);
        return NULL;
    }
    
    /* Create global scope */
    if (!parser_enter_scope(parser, false, false)) {
        free(parser->scope_stack.scopes);
        free(parser->symbol_table.symbols);
        free(parser);
        return NULL;
    }
    
    return parser;
}

void parser_free(ParserState *parser) {
    if (!parser) return;
    
    /* Free all scopes */
    while (parser->scope_stack.scope_count > 0) {
        parser_exit_scope(parser);
    }
    
    /* Free scope stack */
    if (parser->scope_stack.scopes) {
        free(parser->scope_stack.scopes);
    }
    
    /* Free symbol table */
    if (parser->symbol_table.symbols) {
        free(parser->symbol_table.symbols);
    }
    
    /* Free AST tree */
    if (parser->root) {
        ast_node_free(parser->root);
    }
    
    /* Free error message */
    if (parser->last_error) {
        free(parser->last_error);
    }
    
    free(parser);
}

/*
 * AST Node management
 */

ASTNode* ast_node_new(ASTNodeType type, I64 line, I64 column) {
    ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) return NULL;
    
    memset(node, 0, sizeof(ASTNode));
    node->type = type;
    node->line = line;
    node->column = column;
    node->assembly_generated = false;
    node->assembly_code = NULL;
    node->assembly_size = 0;
    node->intermediate = NULL;
    
    return node;
}

void ast_node_free(ASTNode *node) {
    if (!node) return;
    
    /* Free children recursively */
    ASTNode *child = node->children;
    while (child) {
        ASTNode *next = child->next;
        ast_node_free(child);
        child = next;
    }
    
    /* Free node-specific data */
    switch (node->type) {
        case NODE_FUNCTION:
            if (node->data.function.name) free(node->data.function.name);
            if (node->data.function.return_type) free(node->data.function.return_type);
            break;
        case NODE_VARIABLE:
            if (node->data.variable.name) free(node->data.variable.name);
            if (node->data.variable.type) free(node->data.variable.type);
            break;
        case NODE_CALL:
            if (node->data.call.name) free(node->data.call.name);
            if (node->data.call.return_type) free(node->data.call.return_type);
            break;
        case NODE_STRING:
            if (node->data.literal.str_value) free(node->data.literal.str_value);
            break;
        case NODE_IDENTIFIER:
            if (node->data.identifier.name) free(node->data.identifier.name);
            if (node->data.identifier.type) free(node->data.identifier.type);
            break;
        default:
            break;
    }
    
    /* Free assembly code */
    if (node->assembly_code) {
        free(node->assembly_code);
    }
    
    /* Free intermediate code */
    if (node->intermediate) {
        /* Note: Intermediate code cleanup handled by CCmpCtrl */
    }
    
    free(node);
}

void ast_node_add_child(ASTNode *parent, ASTNode *child) {
    if (!parent || !child) return;
    
    child->parent = parent;
    
    if (!parent->children) {
        parent->children = child;
    } else {
        /* Add to end of children list */
        ASTNode *current = parent->children;
        while (current->next) {
            current = current->next;
        }
        current->next = child;
        child->prev = current;
    }
}

void ast_node_add_sibling(ASTNode *node, ASTNode *sibling) {
    if (!node || !sibling) return;
    
    /* Find the end of the sibling list */
    ASTNode *current = node;
    while (current->next) {
        current = current->next;
    }
    
    current->next = sibling;
    sibling->prev = current;
}

/*
 * Utility functions
 */

TokenType parser_current_token(ParserState *parser) {
    if (!parser || !parser->lexer) return TK_EOF;
    return parser->lexer->current_token;
}

TokenType parser_next_token(ParserState *parser) {
    if (!parser || !parser->lexer) return TK_EOF;
    return lex_next_token(parser->lexer);
}

U8* parser_current_token_value(ParserState *parser) {
    if (!parser || !parser->lexer) return NULL;
    return parser->lexer->token_value;
}

I64 parser_current_line(ParserState *parser) {
    if (!parser || !parser->lexer) return 0;
    return parser->lexer->buffer_line;
}

I64 parser_current_column(ParserState *parser) {
    if (!parser || !parser->lexer) return 0;
    return parser->lexer->buffer_column;
}

Bool parser_match_token(ParserState *parser, TokenType token) {
    if (parser_current_token(parser) == token) {
        parser_next_token(parser);
        return true;
    }
    return false;
}

TokenType parser_expect_token(ParserState *parser, TokenType expected) {
    TokenType current = parser_current_token(parser);
    if (current == expected) {
        parser_next_token(parser);
        return current;
    }
    
    parser_expected_error(parser, expected, current);
    return current;
}

/*
 * Error handling
 */

void parser_error(ParserState *parser, U8 *message) {
    if (!parser) return;
    
    parser->error_count++;
    
    if (parser->last_error) {
        free(parser->last_error);
    }
    
    /* Create error message with line/column info */
    I64 line = parser_current_line(parser);
    I64 column = parser_current_column(parser);
    I64 message_len = strlen((char*)message) + 100; /* Extra space for line/column */
    
    parser->last_error = (U8*)malloc(message_len);
    if (parser->last_error) {
        snprintf((char*)parser->last_error, message_len, 
                "Parse error at line %ld, column %ld: %s", 
                line, column, message);
        printf("ERROR: %s\n", parser->last_error);
    }
}

void parser_expected_error(ParserState *parser, TokenType expected, TokenType found) {
    U8 error_msg[256];
    snprintf((char*)error_msg, sizeof(error_msg),
            "Expected token %d, but found token %d", expected, found);
    
    parser_error(parser, error_msg);
}

/*
 * Main parsing functions
 */

ASTNode* parse_program(ParserState *parser) {
    if (!parser) return NULL;
    
    /* Parse program */
    
    /* Create root program node */
    ASTNode *program = ast_node_new(NODE_PROGRAM, 1, 1);
    if (!program) return NULL;
    
    parser->root = program;
    parser->current_node = program;
    
    /* Parse statements until EOF */
    while (parser_current_token(parser) != TK_EOF) {
        ASTNode *stmt = parse_statement(parser);
        if (stmt) {
            ast_node_add_child(program, stmt);
        } else {
            /* Skip to next statement on error */
            parser_next_token(parser);
        }
    }
    
    /* Program parsing complete */
    return program;
}

ASTNode* parse_statement(ParserState *parser) {
    if (!parser) return NULL;
    
    TokenType current = parser_current_token(parser);
    
    /* Parse statement based on current token */
    
    switch (current) {
        case TK_IF:
            return parse_if_statement(parser);
        case TK_WHILE:
            return parse_while_statement(parser);
        case TK_FOR:
            return parse_for_statement(parser);
        case TK_SWITCH:
            return parse_switch_statement(parser);
        case TK_RETURN:
            return parse_return_statement(parser);
        case TK_BREAK:
            return parse_break_statement(parser);
        case TK_CONTINUE:
            return parse_continue_statement(parser);
        case TK_GOTO:
            return parse_goto_statement(parser);
        case '{':
            return parse_block_statement(parser);
        case TK_STR:
        case TK_I64:
        case TK_F64:
        case TK_CHAR_CONST:
        case TK_IDENT:
            /* Try to parse as assignment statement first */
            return parse_assignment_or_expression_statement(parser);
        default:
            /* Check for type tokens */
            if (current >= TK_TYPE_I0 && current <= TK_TYPE_STRING) {
                /* Look ahead to see if this is a function or variable declaration */
                /* Save current position before parsing */
                parser_save_position(parser);
                
                /* Parse type specifier */
                ASTNode *type_node = parse_type_specifier(parser);
                if (!type_node) {
                    parser_restore_position(parser);
                    return parse_expression_statement(parser);
                }
                
                /* Check if next token is identifier */
                if (parser_current_token(parser) != TK_IDENT) {
                    ast_node_free(type_node);
                    parser_restore_position(parser);
                    return parse_variable_declaration(parser);
                }
                
                /* Move past identifier */
                parser_next_token(parser);
                
                /* Check if next token is '(' - indicates function declaration */
                if (parser_current_token(parser) == '(') {
                    /* This is a function declaration - restore position and parse as function */
                    ast_node_free(type_node);
                    parser_restore_position(parser);
                    return parse_function_declaration(parser);
                } else {
                    /* This is a variable declaration - restore position and parse as variable */
                    ast_node_free(type_node);
                    parser_restore_position(parser);
                    return parse_variable_declaration(parser);
                }
            }
            /* Try to parse as expression statement */
            return parse_expression_statement(parser);
    }
}

ASTNode* parse_assignment_or_expression_statement(ParserState *parser) {
    if (!parser) return NULL;
    
    /* Parse identifier */
    if (parser_current_token(parser) != TK_IDENT) {
        return parse_expression_statement(parser);
    }
    
    /* Get the identifier name */
    U8 *var_name = parser_current_token_value(parser);
    
    /* Look ahead to see what comes after the identifier */
    parser_next_token(parser); /* Consume identifier */
    TokenType op = parser_current_token(parser);
    
    if (op == '(') {
        /* This is a function call - parse it */
        /* We're already at the '(' token, so parse the function call */
        ASTNode *call_node = parse_function_call(parser, var_name, parser_current_line(parser), parser_current_column(parser));
        
        /* Expect semicolon after function call */
        parser_expect_token(parser, ';');
        
        return call_node;
    } else if (op == '=') {
        
        /* This is an assignment statement */
        parser_next_token(parser); /* Consume assignment operator */
        
        /* Create assignment node */
        ASTNode *assign_node = ast_node_new(NODE_ASSIGNMENT, parser_current_line(parser), parser_current_column(parser));
        if (!assign_node) return NULL;
        
        /* Create variable node for left side */
        ASTNode *var_node = ast_node_new(NODE_VARIABLE, parser_current_line(parser), parser_current_column(parser));
        if (!var_node) {
            ast_node_free(assign_node);
            return NULL;
        }
        
        var_node->data.identifier.name = var_name;
        var_node->data.identifier.type = (U8*)TK_TYPE_I64; /* Default type for now */
        var_node->data.identifier.is_global = false;
        var_node->data.identifier.is_parameter = false;
        
        /* Parse right side */
        ASTNode *right_expr = parse_additive_expression(parser);
        if (!right_expr) {
            ast_node_free(assign_node);
            ast_node_free(var_node);
            return NULL;
        }
        
        assign_node->data.assignment.left = var_node;
        assign_node->data.assignment.right = right_expr;
        assign_node->data.assignment.op = (op == '=') ? BINOP_ASSIGN : BINOP_ASSIGN; /* For now, just use BINOP_ASSIGN */
        
        /* Expect semicolon */
        if (parser_current_token(parser) == ';') {
            parser_next_token(parser);
        } else {
            parser_error(parser, (U8*)"Expected semicolon after assignment");
            ast_node_free(assign_node);
            return NULL;
        }
        
        return assign_node;
    } else {
        /* This is an expression statement, back up and parse normally */
        /* We need to reset the lexer position, but for now just parse as expression */
        return parse_expression_statement(parser);
    }
    
    return NULL; /* Should never reach here */
}

ASTNode* parse_expression_statement(ParserState *parser) {
    if (!parser) return NULL;
    
    ASTNode *expr = parse_expression(parser);
    if (!expr) return NULL;
    
    /* Expect semicolon */
    parser_expect_token(parser, ';');
    
    return expr;
}

ASTNode* parse_expression(ParserState *parser) {
    if (!parser) return NULL;
    
    /* Parse logical OR expressions (lowest precedence) */
    return parse_logical_or_expression(parser);
}

ASTNode* parse_additive_expression(ParserState *parser) {
    if (!parser) return NULL;
    
    /* Parse multiplicative expressions first */
    ASTNode *left = parse_multiplicative_expression(parser);
    if (!left) return NULL;
    
    /* Parse additive operators (+, -) */
    while (parser_current_token(parser) == '+' || parser_current_token(parser) == '-') {
        TokenType op = parser_current_token(parser);
        parser_next_token(parser); /* Consume operator */
        
        ASTNode *right = parse_multiplicative_expression(parser);
        if (!right) {
            ast_node_free(left);
            return NULL;
        }
        
        /* Create binary operation node */
        ASTNode *binop = ast_node_new(NODE_BINARY_OP, parser_current_line(parser), parser_current_column(parser));
        if (!binop) {
            ast_node_free(left);
            ast_node_free(right);
            return NULL;
        }
        
        binop->data.binary_op.op = (op == '+') ? BINOP_ADD : BINOP_SUB;
        binop->data.binary_op.left = left;
        binop->data.binary_op.right = right;
        
        left = binop;
    }
    
    return left;
}

ASTNode* parse_multiplicative_expression(ParserState *parser) {
    if (!parser) return NULL;
    
    /* Parse unary expressions first */
    ASTNode *left = parse_unary_expression(parser);
    if (!left) return NULL;
    
    /* Parse multiplicative operators (*, /, %) */
    while (parser_current_token(parser) == '*' || parser_current_token(parser) == '/' || parser_current_token(parser) == '%') {
        TokenType op = parser_current_token(parser);
        parser_next_token(parser); /* Consume operator */
        
        ASTNode *right = parse_primary_expression(parser);
        if (!right) {
            ast_node_free(left);
            return NULL;
        }
        
        /* Create binary operation node */
        ASTNode *binop = ast_node_new(NODE_BINARY_OP, parser_current_line(parser), parser_current_column(parser));
        if (!binop) {
            ast_node_free(left);
            ast_node_free(right);
            return NULL;
        }
        
        /* Set operator type */
        switch (op) {
            case '*': binop->data.binary_op.op = BINOP_MUL; break;
            case '/': binop->data.binary_op.op = BINOP_DIV; break;
            case '%': binop->data.binary_op.op = BINOP_MOD; break;
            default: binop->data.binary_op.op = BINOP_ADD; break;
        }
        
        binop->data.binary_op.left = left;
        binop->data.binary_op.right = right;
        
        left = binop;
    }
    
    return left;
}

ASTNode* parse_primary_expression(ParserState *parser) {
    if (!parser) return NULL;
    
    TokenType current = parser_current_token(parser);
    I64 line = parser_current_line(parser);
    I64 column = parser_current_column(parser);
    
    switch (current) {
        case TK_STR: {
            /* String literal */
            ASTNode *node = ast_node_new(NODE_STRING, line, column);
            if (!node) return NULL;
            
            U8 *value = parser_current_token_value(parser);
            if (value) {
                I64 len = strlen((char*)value);
                node->data.literal.str_value = (U8*)malloc(len + 1);
                if (node->data.literal.str_value) {
                    strcpy((char*)node->data.literal.str_value, (char*)value);
                }
            }
            
            parser_next_token(parser);
            return node;
        }
        
        case TK_I64: {
            /* Integer literal */
            ASTNode *node = ast_node_new(NODE_INTEGER, line, column);
            if (!node) return NULL;
            
            U8 *value = parser_current_token_value(parser);
            if (value) {
                node->data.literal.i64_value = strtoll((char*)value, NULL, 0);
            }
            
            parser_next_token(parser);
            return node;
        }
        
        case TK_F64: {
            /* Float literal */
            ASTNode *node = ast_node_new(NODE_FLOAT, line, column);
            if (!node) return NULL;
            
            U8 *value = parser_current_token_value(parser);
            if (value) {
                node->data.literal.f64_value = strtod((char*)value, NULL);
            }
            
            parser_next_token(parser);
            return node;
        }
        
        case TK_CHAR_CONST: {
            /* Character constant */
            ASTNode *node = ast_node_new(NODE_CHAR, line, column);
            if (!node) return NULL;
            
            U8 *value = parser_current_token_value(parser);
            if (value && strlen((char*)value) > 0) {
                node->data.literal.char_value = value[0];
            }
            
            parser_next_token(parser);
            return node;
        }
        
        case TK_IDENT: {
            /* Identifier - could be variable or function call */
            U8 *name = parser_current_token_value(parser);
            parser_next_token(parser);
            
            /* Check if this is a function call */
            if (parser_current_token(parser) == '(') {
                /* Function call */
                return parse_function_call(parser, name, line, column);
            } else {
                /* Variable reference - check if variable is defined in scope */
                if (!parser_is_variable_defined_in_scope(parser, name)) {
                    printf("WARNING: Variable '%s' is not defined in current scope\n", name);
                }
                
                ASTNode *node = ast_node_new(NODE_IDENTIFIER, line, column);
                if (!node) return NULL;
                
                if (name) {
                    I64 len = strlen((char*)name);
                    node->data.identifier.name = (U8*)malloc(len + 1);
                    if (node->data.identifier.name) {
                        strcpy((char*)node->data.identifier.name, (char*)name);
                    }
                }
                
                return node;
            }
        }
        
        case '(': {
            /* Parenthesized expression */
            parser_next_token(parser); /* Skip '(' */
            ASTNode *expr = parse_expression(parser);
            parser_expect_token(parser, ')');
            return expr;
        }
        
        default:
            parser_error(parser, (U8*)"Expected primary expression");
            return NULL;
    }
}

/*
 * Placeholder implementations for other parsing functions
 * These will be implemented as needed
 */

ASTNode* parse_if_statement(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: Parsing if statement, current token: %d\n", parser_current_token(parser));
    
    /* Expect 'if' keyword */
    if (parser_current_token(parser) != TK_IF) {
        parser_error(parser, (U8*)"Expected 'if' keyword");
        return NULL;
    }
    
    I64 line = parser_current_line(parser);
    I64 column = parser_current_column(parser);
    parser_next_token(parser); /* consume 'if' */
    
    /* Expect '(' for condition */
    if (parser_current_token(parser) != '(') {
        parser_error(parser, (U8*)"Expected '(' after 'if'");
        return NULL;
    }
    parser_next_token(parser); /* consume '(' */
    
    /* Parse condition expression */
    ASTNode *condition = parse_expression(parser);
    if (!condition) {
        parser_error(parser, (U8*)"Expected condition expression in if statement");
        return NULL;
    }
    
    /* Expect ')' after condition */
    if (parser_current_token(parser) != ')') {
        parser_error(parser, (U8*)"Expected ')' after if condition");
        ast_node_free(condition);
        return NULL;
    }
    parser_next_token(parser); /* consume ')' */
    
    /* Parse then statement */
    ASTNode *then_stmt = parse_statement(parser);
    if (!then_stmt) {
        parser_error(parser, (U8*)"Expected statement after if condition");
        ast_node_free(condition);
        return NULL;
    }
    
    /* Create if statement node */
    ASTNode *if_node = ast_node_new(NODE_IF_STMT, line, column);
    if (!if_node) {
        ast_node_free(condition);
        ast_node_free(then_stmt);
        return NULL;
    }
    
    /* Set if statement data */
    if_node->data.if_stmt.condition = condition;
    if_node->data.if_stmt.then_stmt = then_stmt;
    if_node->data.if_stmt.else_stmt = NULL;
    
    /* Check for optional else clause */
    if (parser_current_token(parser) == TK_ELSE) {
        parser_next_token(parser); /* consume 'else' */
        ASTNode *else_stmt = parse_statement(parser);
        if (else_stmt) {
            if_node->data.if_stmt.else_stmt = else_stmt;
        }
    }
    
    printf("DEBUG: If statement parsed successfully\n");
    return if_node;
}

ASTNode* parse_while_statement(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: Parsing while statement, current token: %d\n", parser_current_token(parser));
    
    /* Expect 'while' keyword */
    if (parser_current_token(parser) != TK_WHILE) {
        parser_error(parser, (U8*)"Expected 'while' keyword");
        return NULL;
    }
    
    I64 line = parser_current_line(parser);
    I64 column = parser_current_column(parser);
    parser_next_token(parser); /* consume 'while' */
    
    /* Expect '(' for condition */
    if (parser_current_token(parser) != '(') {
        parser_error(parser, (U8*)"Expected '(' after 'while'");
        return NULL;
    }
    parser_next_token(parser); /* consume '(' */
    
    /* Parse condition expression */
    ASTNode *condition = parse_expression(parser);
    if (!condition) {
        parser_error(parser, (U8*)"Expected condition expression in while statement");
        return NULL;
    }
    
    /* Expect ')' after condition */
    if (parser_current_token(parser) != ')') {
        parser_error(parser, (U8*)"Expected ')' after while condition");
        ast_node_free(condition);
        return NULL;
    }
    parser_next_token(parser); /* consume ')' */
    
    /* Parse loop body statement */
    ASTNode *body_stmt = parse_statement(parser);
    if (!body_stmt) {
        parser_error(parser, (U8*)"Expected statement after while condition");
        ast_node_free(condition);
        return NULL;
    }
    
    /* Create while statement node */
    ASTNode *while_node = ast_node_new(NODE_WHILE_STMT, line, column);
    if (!while_node) {
        ast_node_free(condition);
        ast_node_free(body_stmt);
        return NULL;
    }
    
    /* Set while statement data */
    while_node->data.while_stmt.condition = condition;
    while_node->data.while_stmt.body_stmt = body_stmt;
    
    printf("DEBUG: While statement parsed successfully\n");
    return while_node;
}

ASTNode* parse_for_statement(ParserState *parser) {
    parser_error(parser, (U8*)"for statement not yet implemented");
    return NULL;
}

ASTNode* parse_switch_statement(ParserState *parser) {
    parser_error(parser, (U8*)"switch statement not yet implemented");
    return NULL;
}

ASTNode* parse_return_statement(ParserState *parser) {
    if (!parser) return NULL;
    
    /* Parse return statement: return [expression]; */
    
    /* Check if we have 'return' token */
    if (parser_current_token(parser) != TK_RETURN) {
        parser_error(parser, (U8*)"Expected 'return' keyword");
        return NULL;
    }
    
    I64 line = parser_current_line(parser);
    I64 column = parser_current_column(parser);
    
    parser_next_token(parser); /* consume 'return' */
    
    /* Create return statement node */
    ASTNode *return_node = ast_node_new(NODE_RETURN, line, column);
    if (!return_node) return NULL;
    
    /* Initialize return statement data */
    return_node->data.return_stmt.expression = NULL;
    return_node->data.return_stmt.return_value = 0;
    return_node->data.return_stmt.return_type = NULL;
    
    /* Check if there's an expression after 'return' */
    if (parser_current_token(parser) != ';' && parser_current_token(parser) != TK_EOF) {
        /* Check if it's a simple integer literal */
        if (parser_current_token(parser) == TK_I64) {
            /* Simple integer literal - set return_value directly */
            I64 value = strtoll((char*)parser_current_token_value(parser), NULL, 10);
            return_node->data.return_stmt.return_value = value;
            printf("DEBUG: Parsed simple return value: %lld\n", value);
            parser_next_token(parser); /* consume the integer */
        } else {
            /* Complex expression - parse it */
            ASTNode *expr = parse_expression(parser);
            if (expr) {
                return_node->data.return_stmt.expression = expr;
                printf("DEBUG: Parsed complex return expression\n");
            }
        }
    }
    
    /* Expect semicolon */
    if (parser_current_token(parser) != ';') {
        parser_error(parser, (U8*)"Expected ';' after return statement");
        ast_node_free(return_node);
        return NULL;
    }
    
    parser_next_token(parser); /* consume ';' */
    
    return return_node;
}

ASTNode* parse_break_statement(ParserState *parser) {
    parser_error(parser, (U8*)"break statement not yet implemented");
    return NULL;
}

ASTNode* parse_continue_statement(ParserState *parser) {
    parser_error(parser, (U8*)"continue statement not yet implemented");
    return NULL;
}

ASTNode* parse_goto_statement(ParserState *parser) {
    parser_error(parser, (U8*)"goto statement not yet implemented");
    return NULL;
}

ASTNode* parse_block_statement(ParserState *parser) {
    if (!parser) return NULL;
    
    /* Expect opening brace */
    if (parser_current_token(parser) != '{') {
        parser_error(parser, (U8*)"Expected '{' to start block");
        return NULL;
    }
    parser_next_token(parser); /* consume '{' */
    
    /* Create block node */
    ASTNode *block_node = ast_node_new(NODE_BLOCK, parser_current_line(parser), parser_current_column(parser));
    if (!block_node) {
        return NULL;
    }
    
    /* Initialize block data */
    block_node->data.block.statements = NULL;
    block_node->data.block.statement_count = 0;
    block_node->data.block.local_vars = NULL;
    block_node->data.block.local_var_count = 0;
    
    /* Enter block scope (only for standalone blocks, not function bodies) */
    ScopeLevel *current_scope = parser_get_current_scope(parser);
    Bool entered_block_scope = false;
    if (!current_scope || !current_scope->is_function_scope) {
        /* This is a standalone block, create a new scope */
        if (parser_enter_scope(parser, false, true)) {
            entered_block_scope = true;
        }
    }
    
    /* Parse statements until we find the closing brace */
    while (parser_current_token(parser) != '}') {
        /* Check for end of file (should not happen in valid code) */
        if (parser_current_token(parser) == TK_EOF) {
            parser_error(parser, (U8*)"Expected '}' to close block");
            if (entered_block_scope) {
                parser_exit_scope(parser);
            }
            ast_node_free(block_node);
            return NULL;
        }
        
        /* Parse a statement */
        ASTNode *stmt = parse_statement(parser);
        if (!stmt) {
            /* If statement parsing fails, we might still have a valid block with other statements */
            /* Skip to next token and continue */
            parser_next_token(parser);
            continue;
        }
        
        /* Add statement to block */
        ast_node_add_child(block_node, stmt);
        
        /* Also set the block's statements field for block-specific access */
        if (!block_node->data.block.statements) {
            block_node->data.block.statements = stmt;
        } else {
            /* Add to end of statements list */
            ASTNode *current = block_node->data.block.statements;
            while (current->next) {
                current = current->next;
            }
            current->next = stmt;
        }
        block_node->data.block.statement_count++;
    }
    
    /* Consume the closing brace */
    parser_next_token(parser);
    
    /* Exit block scope if we entered one */
    if (entered_block_scope) {
        parser_exit_scope(parser);
    }
    
    return block_node;
}

ASTNode* parse_function_call(ParserState *parser, U8 *name, I64 line, I64 column) {
    if (!parser || !name) return NULL;
    
    /* Create function call node */
    ASTNode *call_node = ast_node_new(NODE_CALL, line, column);
    if (!call_node) {
        return NULL;
    }
    
    /* Set function name */
    I64 len = strlen((char*)name);
    call_node->data.call.name = (U8*)malloc(len + 1);
    if (call_node->data.call.name) {
        strcpy((char*)call_node->data.call.name, (char*)name);
    }
    
    /* Initialize call data */
    call_node->data.call.arguments = NULL;
    call_node->data.call.return_type = NULL;
    call_node->data.call.return_reg = REG_NONE;
    call_node->data.call.arg_count = 0;
    call_node->data.call.stack_cleanup = 0;
    
    /* Parse argument list */
    ASTNode *arguments = parse_argument_list(parser);
    if (arguments) {
        call_node->data.call.arguments = arguments;
        call_node->data.call.arg_count = arguments->data.block.statement_count;
        printf("DEBUG: Function call has %d arguments\n", call_node->data.call.arg_count);
    } else {
        call_node->data.call.arguments = NULL;
        call_node->data.call.arg_count = 0;
        printf("DEBUG: Function call has no arguments\n");
    }
    
    return call_node;
}

ASTNode* parse_variable_declaration(ParserState *parser) {
    if (!parser) return NULL;
    
    /* Parse type specifier */
    ASTNode *type_node = parse_type_specifier(parser);
    if (!type_node) {
        parser_error(parser, "Expected type specifier in variable declaration");
        return NULL;
    }
    
    /* Parse identifier */
    if (parser_current_token(parser) != TK_IDENT) {
        parser_error(parser, "Expected identifier after type specifier");
        ast_node_free(type_node);
        return NULL;
    }
    
    ASTNode *var_node = ast_node_new(NODE_VARIABLE, parser_current_line(parser), parser_current_column(parser));
    if (!var_node) {
        ast_node_free(type_node);
        return NULL;
    }
    
    /* Set variable information */
    var_node->data.identifier.name = parser_current_token_value(parser);
    var_node->data.identifier.type = (U8*)type_node->data.type_specifier.type;  /* Cast for now */
    var_node->data.identifier.is_global = false; /* Default to local */
    var_node->data.identifier.is_parameter = false;
    
    /* Move to next token */
    parser_next_token(parser);
    
    /* Add variable to current scope */
    ScopeLevel *current_scope = parser_get_current_scope(parser);
    if (current_scope) {
        if (!scope_add_variable(current_scope, var_node)) {
            printf("WARNING: Failed to add variable to scope\n");
        }
    }
    
    /* Check for assignment */
    if (parser_current_token(parser) == '=') {
        parser_next_token(parser); /* Consume '=' */
        
        /* Parse assignment expression */
        ASTNode *assign_node = ast_node_new(NODE_ASSIGNMENT, parser_current_line(parser), parser_current_column(parser));
        if (assign_node) {
            /* Left side: variable */
            assign_node->data.assignment.left = var_node;
            assign_node->data.assignment.op = BINOP_ASSIGN;
            
            /* Right side: expression (for now, just primary expression) */
            assign_node->data.assignment.right = parse_primary_expression(parser);
            if (!assign_node->data.assignment.right) {
                ast_node_free(assign_node);
                ast_node_free(type_node);
                return NULL;
            }
            
            ast_node_free(type_node);
            
            /* Expect semicolon after assignment */
            if (parser_current_token(parser) == ';') {
                parser_next_token(parser);
            } else {
                parser_error(parser, (U8*)"Expected semicolon after assignment");
                ast_node_free(assign_node);
                return NULL;
            }
            
            /* Add variable to symbol table */
            parser_add_symbol(parser, var_node->data.identifier.name, var_node);
            
            return assign_node;
        }
    }
    
    /* No assignment, just declaration */
    ast_node_free(type_node);
    
    /* Expect semicolon after variable declaration */
    if (parser_current_token(parser) == ';') {
        parser_next_token(parser);
    } else {
        parser_error(parser, (U8*)"Expected semicolon after variable declaration");
        ast_node_free(var_node);
        return NULL;
    }
    
    /* Add variable to symbol table */
    parser_add_symbol(parser, var_node->data.identifier.name, var_node);
    
    return var_node;
}

ASTNode* parse_function_declaration(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: Parsing function declaration, current token: %d\n", parser_current_token(parser));
    
    /* Parse return type */
    ASTNode *return_type = parse_type_specifier(parser);
    if (!return_type) {
        printf("DEBUG: Failed to parse return type, current token: %d\n", parser_current_token(parser));
        parser_error(parser, (U8*)"Expected return type in function declaration");
        return NULL;
    }
    printf("DEBUG: Successfully parsed return type\n");
    
    /* Parse function name */
    printf("DEBUG: Looking for function name, current token: %d\n", parser_current_token(parser));
    if (parser_current_token(parser) != TK_IDENT) {
        printf("DEBUG: Expected TK_IDENT (%d) but got %d\n", TK_IDENT, parser_current_token(parser));
        parser_error(parser, (U8*)"Expected function name after return type");
        ast_node_free(return_type);
        return NULL;
    }
    
    U8 *func_name = parser_current_token_value(parser);
    printf("DEBUG: Found function name: %s\n", func_name ? (char*)func_name : "NULL");
    parser_next_token(parser);
    
    /* Parse parameter list */
    printf("DEBUG: Parsing parameter list, current token: %d\n", parser_current_token(parser));
    ASTNode *parameters = parse_parameter_list(parser);
    printf("DEBUG: Parameter list parsing completed\n");
    
    /* Parse function body */
    printf("DEBUG: Looking for '{', current token: %d\n", parser_current_token(parser));
    if (parser_current_token(parser) != '{') {
        printf("DEBUG: Expected '{' (123) but got %d\n", parser_current_token(parser));
        parser_error(parser, (U8*)"Expected '{' for function body");
        ast_node_free(return_type);
        return NULL;
    }
    printf("DEBUG: Found opening '{' - not consuming it, letting parse_block_statement handle it\n");
    
    /* Create function node */
    ASTNode *func_node = ast_node_new(NODE_FUNCTION, parser_current_line(parser), parser_current_column(parser));
    if (!func_node) {
        ast_node_free(return_type);
        return NULL;
    }
    
    /* Set function information */
    func_node->data.function.name = func_name;
    func_node->data.function.return_type = (U8*)return_type->data.type_specifier.type; /* Cast for now */
    func_node->data.function.parameters = NULL; /* TODO: Parse parameters */
    func_node->data.function.body = NULL; /* TODO: Parse function body */
    func_node->data.function.is_extern = false;
    func_node->data.function.is_public = false;
    func_node->data.function.is_reg = false;
    func_node->data.function.is_interrupt = false;
    
    /* Enter function scope */
    if (!parser_enter_scope(parser, true, false)) {
        ast_node_free(func_node);
        ast_node_free(return_type);
        return NULL;
    }
    
    /* Parse function body statements */
    ASTNode *body_node = parse_block_statement(parser);
    if (!body_node) {
        parser_exit_scope(parser);
        ast_node_free(func_node);
        ast_node_free(return_type);
        return NULL;
    }
    
    func_node->data.function.body = body_node;
    
    /* Exit function scope */
    parser_exit_scope(parser);
    
    /* Add function to symbol table */
    parser_add_symbol(parser, func_name, func_node);
    
    printf("DEBUG: Function declaration parsed successfully: %s\n", func_name ? (char*)func_name : "unnamed");
    return func_node;
}

/* Placeholder implementations for all other functions */
ASTNode* parse_assignment_expression(ParserState *parser) { return NULL; }
ASTNode* parse_conditional_expression(ParserState *parser) { return NULL; }
ASTNode* parse_logical_or_expression(ParserState *parser) {
    if (!parser) return NULL;
    
    /* Parse logical AND expressions first */
    ASTNode *left = parse_logical_and_expression(parser);
    if (!left) return NULL;
    
    /* Parse logical OR operators (||) */
    while (parser_current_token(parser) == TK_OR_OR) {
        parser_next_token(parser); /* Consume || */
        
        ASTNode *right = parse_logical_and_expression(parser);
        if (!right) {
            ast_node_free(left);
            return NULL;
        }
        
        /* Create binary operation node */
        ASTNode *binop = ast_node_new(NODE_BINARY_OP, parser_current_line(parser), parser_current_column(parser));
        if (!binop) {
            ast_node_free(left);
            ast_node_free(right);
            return NULL;
        }
        
        binop->data.binary_op.op = BINOP_OR_OR;
        binop->data.binary_op.left = left;
        binop->data.binary_op.right = right;
        
        left = binop;
    }
    
    return left;
}
ASTNode* parse_logical_and_expression(ParserState *parser) {
    if (!parser) return NULL;
    
    /* Parse equality expressions first */
    ASTNode *left = parse_equality_expression(parser);
    if (!left) return NULL;
    
    /* Parse logical AND operators (&&) */
    while (parser_current_token(parser) == TK_AND_AND) {
        parser_next_token(parser); /* Consume && */
        
        ASTNode *right = parse_equality_expression(parser);
        if (!right) {
            ast_node_free(left);
            return NULL;
        }
        
        /* Create binary operation node */
        ASTNode *binop = ast_node_new(NODE_BINARY_OP, parser_current_line(parser), parser_current_column(parser));
        if (!binop) {
            ast_node_free(left);
            ast_node_free(right);
            return NULL;
        }
        
        binop->data.binary_op.op = BINOP_AND_AND;
        binop->data.binary_op.left = left;
        binop->data.binary_op.right = right;
        
        left = binop;
    }
    
    return left;
}
ASTNode* parse_bitwise_or_expression(ParserState *parser) { return NULL; }
ASTNode* parse_bitwise_xor_expression(ParserState *parser) { return NULL; }
ASTNode* parse_bitwise_and_expression(ParserState *parser) { return NULL; }
ASTNode* parse_equality_expression(ParserState *parser) {
    if (!parser) return NULL;
    
    /* Parse relational expressions first */
    ASTNode *left = parse_relational_expression(parser);
    if (!left) return NULL;
    
    /* Parse equality operators (==, !=) */
    while (parser_current_token(parser) == TK_EQU_EQU || parser_current_token(parser) == TK_NOT_EQU) {
        TokenType op = parser_current_token(parser);
        parser_next_token(parser); /* Consume operator */
        
        ASTNode *right = parse_relational_expression(parser);
        if (!right) {
            ast_node_free(left);
            return NULL;
        }
        
        /* Create binary operation node */
        ASTNode *binop = ast_node_new(NODE_BINARY_OP, parser_current_line(parser), parser_current_column(parser));
        if (!binop) {
            ast_node_free(left);
            ast_node_free(right);
            return NULL;
        }
        
        /* Set operator type */
        switch (op) {
            case TK_EQU_EQU: binop->data.binary_op.op = BINOP_EQ; break;
            case TK_NOT_EQU: binop->data.binary_op.op = BINOP_NE; break;
            default: binop->data.binary_op.op = BINOP_EQ; break;
        }
        
        binop->data.binary_op.left = left;
        binop->data.binary_op.right = right;
        
        left = binop;
    }
    
    return left;
}
ASTNode* parse_relational_expression(ParserState *parser) {
    if (!parser) return NULL;
    
    /* Parse shift expressions first */
    ASTNode *left = parse_shift_expression(parser);
    if (!left) return NULL;
    
    /* Parse relational operators (<, >, <=, >=) */
    while (parser_current_token(parser) == '<' || parser_current_token(parser) == '>' ||
           parser_current_token(parser) == TK_LESS_EQU || parser_current_token(parser) == TK_GREATER_EQU) {
        TokenType op = parser_current_token(parser);
        parser_next_token(parser); /* Consume operator */
        
        ASTNode *right = parse_shift_expression(parser);
        if (!right) {
            ast_node_free(left);
            return NULL;
        }
        
        /* Create binary operation node */
        ASTNode *binop = ast_node_new(NODE_BINARY_OP, parser_current_line(parser), parser_current_column(parser));
        if (!binop) {
            ast_node_free(left);
            ast_node_free(right);
            return NULL;
        }
        
        /* Set operator type */
        switch (op) {
            case '<': binop->data.binary_op.op = BINOP_LT; break;
            case '>': binop->data.binary_op.op = BINOP_GT; break;
            case TK_LESS_EQU: binop->data.binary_op.op = BINOP_LE; break;
            case TK_GREATER_EQU: binop->data.binary_op.op = BINOP_GE; break;
            default: binop->data.binary_op.op = BINOP_LT; break;
        }
        
        binop->data.binary_op.left = left;
        binop->data.binary_op.right = right;
        
        left = binop;
    }
    
    return left;
}
ASTNode* parse_shift_expression(ParserState *parser) {
    if (!parser) return NULL;
    
    /* Parse additive expressions first */
    ASTNode *left = parse_additive_expression(parser);
    if (!left) return NULL;
    
    /* Parse shift operators (<<, >>) */
    while (parser_current_token(parser) == TK_SHL || parser_current_token(parser) == TK_SHR) {
        TokenType op = parser_current_token(parser);
        parser_next_token(parser); /* Consume operator */
        
        ASTNode *right = parse_additive_expression(parser);
        if (!right) {
            ast_node_free(left);
            return NULL;
        }
        
        /* Create binary operation node */
        ASTNode *binop = ast_node_new(NODE_BINARY_OP, parser_current_line(parser), parser_current_column(parser));
        if (!binop) {
            ast_node_free(left);
            ast_node_free(right);
            return NULL;
        }
        
        /* Set operator type */
        switch (op) {
            case TK_SHL: binop->data.binary_op.op = BINOP_SHL; break;
            case TK_SHR: binop->data.binary_op.op = BINOP_SHR; break;
            default: binop->data.binary_op.op = BINOP_SHL; break;
        }
        
        binop->data.binary_op.left = left;
        binop->data.binary_op.right = right;
        
        left = binop;
    }
    
    return left;
}
/* parse_additive_expression and parse_multiplicative_expression implemented above */
ASTNode* parse_unary_expression(ParserState *parser) {
    if (!parser) return NULL;
    
    TokenType current = parser_current_token(parser);
    I64 line = parser_current_line(parser);
    I64 column = parser_current_column(parser);
    
    /* Check for unary operators */
    switch (current) {
        case '!': {
            /* Logical NOT (!) */
            parser_next_token(parser); /* Consume ! */
            
            ASTNode *operand = parse_unary_expression(parser);
            if (!operand) return NULL;
            
            ASTNode *unop = ast_node_new(NODE_UNARY_OP, line, column);
            if (!unop) {
                ast_node_free(operand);
                return NULL;
            }
            
            unop->data.unary_op.op = UNOP_NOT;
            unop->data.unary_op.operand = operand;
            
            return unop;
        }
        
        case '~': {
            /* Bitwise NOT (~) */
            parser_next_token(parser); /* Consume ~ */
            
            ASTNode *operand = parse_unary_expression(parser);
            if (!operand) return NULL;
            
            ASTNode *unop = ast_node_new(NODE_UNARY_OP, line, column);
            if (!unop) {
                ast_node_free(operand);
                return NULL;
            }
            
            unop->data.unary_op.op = UNOP_BITNOT;
            unop->data.unary_op.operand = operand;
            
            return unop;
        }
        
        case '+': {
            /* Unary plus (+) */
            parser_next_token(parser); /* Consume + */
            
            ASTNode *operand = parse_unary_expression(parser);
            if (!operand) return NULL;
            
            ASTNode *unop = ast_node_new(NODE_UNARY_OP, line, column);
            if (!unop) {
                ast_node_free(operand);
                return NULL;
            }
            
            unop->data.unary_op.op = UNOP_PLUS;
            unop->data.unary_op.operand = operand;
            
            return unop;
        }
        
        case '-': {
            /* Unary minus (-) */
            parser_next_token(parser); /* Consume - */
            
            ASTNode *operand = parse_unary_expression(parser);
            if (!operand) return NULL;
            
            ASTNode *unop = ast_node_new(NODE_UNARY_OP, line, column);
            if (!unop) {
                ast_node_free(operand);
                return NULL;
            }
            
            unop->data.unary_op.op = UNOP_MINUS;
            unop->data.unary_op.operand = operand;
            
            return unop;
        }
        
        case TK_PLUS_PLUS: {
            /* Pre-increment (++x) */
            parser_next_token(parser); /* Consume ++ */
            
            ASTNode *operand = parse_unary_expression(parser);
            if (!operand) return NULL;
            
            ASTNode *unop = ast_node_new(NODE_UNARY_OP, line, column);
            if (!unop) {
                ast_node_free(operand);
                return NULL;
            }
            
            unop->data.unary_op.op = UNOP_INC;
            unop->data.unary_op.operand = operand;
            
            return unop;
        }
        
        case TK_MINUS_MINUS: {
            /* Pre-decrement (--x) */
            parser_next_token(parser); /* Consume -- */
            
            ASTNode *operand = parse_unary_expression(parser);
            if (!operand) return NULL;
            
            ASTNode *unop = ast_node_new(NODE_UNARY_OP, line, column);
            if (!unop) {
                ast_node_free(operand);
                return NULL;
            }
            
            unop->data.unary_op.op = UNOP_DEC;
            unop->data.unary_op.operand = operand;
            
            return unop;
        }
        
        case '&': {
            /* Address-of (&x) */
            parser_next_token(parser); /* Consume & */
            
            ASTNode *operand = parse_unary_expression(parser);
            if (!operand) return NULL;
            
            ASTNode *unop = ast_node_new(NODE_UNARY_OP, line, column);
            if (!unop) {
                ast_node_free(operand);
                return NULL;
            }
            
            unop->data.unary_op.op = UNOP_ADDR;
            unop->data.unary_op.operand = operand;
            
            return unop;
        }
        
        case '*': {
            /* Dereference (*x) */
            parser_next_token(parser); /* Consume * */
            
            ASTNode *operand = parse_unary_expression(parser);
            if (!operand) return NULL;
            
            ASTNode *unop = ast_node_new(NODE_UNARY_OP, line, column);
            if (!unop) {
                ast_node_free(operand);
                return NULL;
            }
            
            unop->data.unary_op.op = UNOP_DEREF;
            unop->data.unary_op.operand = operand;
            
            return unop;
        }
        
        default:
            /* No unary operator, parse postfix expression */
            return parse_postfix_expression(parser);
    }
}
ASTNode* parse_postfix_expression(ParserState *parser) {
    if (!parser) return NULL;
    
    /* For now, just parse primary expressions */
    /* TODO: Add support for postfix operators like [], (), ->, ++, -- */
    return parse_primary_expression(parser);
}
ASTNode* parse_parameter_list(ParserState *parser) {
    if (!parser) return NULL;
    
    /* Parse parameter list: (type name, type name, ...) */
    
    /* Check if we have parameters */
    if (parser_current_token(parser) != '(') {
        return NULL;
    }
    
    parser_next_token(parser); /* consume '(' */
    
    /* Create a dummy node to represent the parameter list */
    /* For now, we'll just count parameters and skip them */
    ASTNode *param_list = ast_node_new(NODE_BLOCK, parser_current_line(parser), parser_current_column(parser));
    if (!param_list) return NULL;
    
    /* Initialize parameter list */
    param_list->data.block.statements = NULL;
    param_list->data.block.statement_count = 0;
    param_list->data.block.local_vars = NULL;
    param_list->data.block.local_var_count = 0;
    
    I64 param_count = 0;
    
    /* Parse parameters */
    while (parser_current_token(parser) != ')' && parser_current_token(parser) != TK_EOF) {
        /* Skip whitespace and commas */
        if (parser_current_token(parser) == ',') {
            parser_next_token(parser);
            continue;
        }
        
        /* Parse parameter type */
        ASTNode *param_type = parse_type_specifier(parser);
        if (!param_type) {
            printf("DEBUG: Failed to parse parameter type\n");
            break;
        }
        
        /* Parse parameter name */
        if (parser_current_token(parser) != TK_IDENT) {
            printf("DEBUG: Expected parameter name, got token %d\n", parser_current_token(parser));
            ast_node_free(param_type);
            break;
        }
        
        U8 *param_name = parser_current_token_value(parser);
        printf("DEBUG: Parsed parameter: %s\n", param_name ? (char*)param_name : "NULL");
        parser_next_token(parser);
        
        /* Create parameter variable node */
        ASTNode *param_var = ast_node_new(NODE_VARIABLE, parser_current_line(parser), parser_current_column(parser));
        if (param_var) {
            param_var->data.variable.name = param_name;
            param_var->data.variable.type = (U8*)param_type->data.type_specifier.type;
            param_var->data.variable.is_parameter = true;
            param_var->data.variable.parameter_index = param_count;
            
            /* Add parameter to scope */
            if (!scope_add_variable(parser_get_current_scope(parser), param_var)) {
                printf("WARNING: Failed to add parameter to scope\n");
            }
        }
        
        param_count++;
        ast_node_free(param_type);
    }
    
    /* Store parameter count in local_var_count field as a temporary measure */
    param_list->data.block.local_var_count = param_count;
    
    if (parser_current_token(parser) == ')') {
        parser_next_token(parser); /* consume ')' */
    }
    
    /* Parsed %ld parameters */
    return param_list;
}
ASTNode* parse_argument_list(ParserState *parser) {
    if (!parser) return NULL;
    
    /* Parse argument list: (expr, expr, ...) */
    
    /* Check if we have arguments */
    if (parser_current_token(parser) != '(') {
        return NULL;
    }
    
    parser_next_token(parser); /* consume '(' */
    
    /* Create a dummy node to represent the argument list */
    /* For now, we'll just count arguments and skip them */
    ASTNode *arg_list = ast_node_new(NODE_BLOCK, parser_current_line(parser), parser_current_column(parser));
    if (!arg_list) return NULL;
    
    /* Initialize argument list */
    arg_list->data.block.statements = NULL;
    arg_list->data.block.statement_count = 0;
    arg_list->data.block.local_vars = NULL;
    arg_list->data.block.local_var_count = 0;
    
    I64 arg_count = 0;
    
    /* Parse arguments */
    while (parser_current_token(parser) != ')' && parser_current_token(parser) != TK_EOF) {
        /* Skip whitespace and commas */
        if (parser_current_token(parser) == ',') {
            parser_next_token(parser);
            continue;
        }
        
        /* Parse argument expression */
        ASTNode *arg_expr = parse_expression(parser);
        if (arg_expr) {
            printf("DEBUG: Parsed function call argument: type %d\n", arg_expr->type);
            
            /* Add argument to argument list */
            if (!arg_list->data.block.statements) {
                arg_list->data.block.statements = arg_expr;
            } else {
                /* Add to end of arguments list */
                ASTNode *current = arg_list->data.block.statements;
                while (current->next) {
                    current = current->next;
                }
                current->next = arg_expr;
            }
            arg_list->data.block.statement_count++;
            arg_count++;
        } else {
            printf("DEBUG: Failed to parse function call argument\n");
            break;
        }
    }
    
    /* Store argument count in local_var_count field as a temporary measure */
    arg_list->data.block.local_var_count = arg_count;
    
    if (parser_current_token(parser) == ')') {
        parser_next_token(parser); /* consume ')' */
    }
    
    /* Parsed %ld arguments */
    return arg_list;
}
ASTNode* parse_type_specifier(ParserState *parser) {
    if (!parser) return NULL;
    
    TokenType current = parser_current_token(parser);
    
    /* Check for type tokens */
    if (current >= TK_TYPE_I0 && current <= TK_TYPE_STRING) {
        ASTNode *type_node = ast_node_new(NODE_TYPE_SPECIFIER, parser_current_line(parser), parser_current_column(parser));
        if (!type_node) return NULL;
        
        /* Set the type information */
        type_node->data.type_specifier.type = current;
        
        /* Move to next token */
        parser_next_token(parser);
        
        return type_node;
    }
    
    /* No type found */
    return NULL;
}
ASTNode* parse_assembly_block(ParserState *parser) { return NULL; }
ASTNode* parse_assembly_instruction(ParserState *parser) { return NULL; }
ASTNode* parse_assembly_operand(ParserState *parser) { return NULL; }
ASTNode* parse_assembly_register(ParserState *parser) { return NULL; }
ASTNode* parse_assembly_memory(ParserState *parser) { return NULL; }
ASTNode* parse_assembly_immediate(ParserState *parser) { return NULL; }
ASTNode* parse_assembly_label(ParserState *parser) { return NULL; }
ASTNode* parse_range_expression(ParserState *parser) { return NULL; }
ASTNode* parse_dollar_expression(ParserState *parser) { return NULL; }
ASTNode* parse_class_definition(ParserState *parser) { return NULL; }
ASTNode* parse_union_definition(ParserState *parser) { return NULL; }
ASTNode* parse_public_declaration(ParserState *parser) { return NULL; }
ASTNode* parse_extern_declaration(ParserState *parser) { return NULL; }
ASTNode* parse_import_declaration(ParserState *parser) { return NULL; }
ASTNode* parse_try_catch_block(ParserState *parser) { return NULL; }
ASTNode* parse_throw_statement(ParserState *parser) { return NULL; }
ASTNode* parse_type_declaration(ParserState *parser) { return NULL; }
ASTNode* parse_type_cast(ParserState *parser) { return NULL; }
ASTNode* parse_type_reference(ParserState *parser) { return NULL; }
ASTNode* parse_type_dereference(ParserState *parser) { return NULL; }
ASTNode* parse_array_access(ParserState *parser) { return NULL; }
ASTNode* parse_member_access(ParserState *parser) { return NULL; }
ASTNode* parse_pointer_arithmetic(ParserState *parser) { return NULL; }

void parser_warning(ParserState *parser, U8 *message) {
    if (!parser) return;
    parser->warning_count++;
    printf("WARNING: %s\n", message);
}

void parser_add_symbol(ParserState *parser, U8 *name, ASTNode *declaration) {
    if (!parser || !name || !declaration) return;
    
    /* Check if symbol table needs to be initialized */
    if (!parser->symbol_table.symbols) {
        parser->symbol_table.capacity = 16; /* Initial capacity */
        parser->symbol_table.symbols = (ASTNode**)malloc(parser->symbol_table.capacity * sizeof(ASTNode*));
        if (!parser->symbol_table.symbols) {
            printf("ERROR: Failed to allocate symbol table\n");
            return;
        }
        parser->symbol_table.count = 0;
    }
    
    /* Check if we need to expand the symbol table */
    if (parser->symbol_table.count >= parser->symbol_table.capacity) {
        I64 new_capacity = parser->symbol_table.capacity * 2;
        ASTNode **new_symbols = (ASTNode**)realloc(parser->symbol_table.symbols, 
                                                   new_capacity * sizeof(ASTNode*));
        if (!new_symbols) {
            printf("ERROR: Failed to expand symbol table\n");
            return;
        }
        parser->symbol_table.symbols = new_symbols;
        parser->symbol_table.capacity = new_capacity;
    }
    
    /* Add the symbol to the table */
    parser->symbol_table.symbols[parser->symbol_table.count] = declaration;
    parser->symbol_table.count++;
    
    /* Added symbol '%s' to symbol table (count: %ld) */
}

ASTNode* parser_lookup_symbol(ParserState *parser, U8 *name) {
    if (!parser || !name) return NULL;
    
    /* Search through the symbol table */
    for (I64 i = 0; i < parser->symbol_table.count; i++) {
        ASTNode *symbol = parser->symbol_table.symbols[i];
        if (!symbol) continue;
        
        /* Check if this symbol matches the name */
        U8 *symbol_name = NULL;
        
        switch (symbol->type) {
            case NODE_FUNCTION:
                symbol_name = symbol->data.function.name;
                break;
            case NODE_VARIABLE:
                symbol_name = symbol->data.variable.name;
                break;
            case NODE_IDENTIFIER:
                symbol_name = symbol->data.identifier.name;
                break;
            default:
                continue;
        }
        
        if (symbol_name && strcmp((char*)symbol_name, (char*)name) == 0) {
            /* Found symbol '%s' in symbol table */
            return symbol;
        }
    }
    
    /* Symbol '%s' not found in symbol table */
    return NULL;
}

Bool parser_is_symbol_defined(ParserState *parser, U8 *name) {
    return parser_lookup_symbol(parser, name) != NULL;
}

/* Parser position management functions */
void parser_save_position(ParserState *parser) {
    if (!parser) return;
    
    /* Save current lexer state */
    parser->saved_buffer_pos = parser->lexer->buffer_pos;
    parser->saved_buffer_line = parser->lexer->buffer_line;
    parser->saved_buffer_column = parser->lexer->buffer_column;
    parser->saved_current_token = parser->lexer->current_token;
    parser->saved_token_value = parser->lexer->token_value;
    parser->saved_token_length = parser->lexer->token_length;
    parser->position_saved = true;
    
    printf("DEBUG: Saved parser position at buffer %ld, token %d\n", 
           parser->saved_buffer_pos, parser->saved_current_token);
}

void parser_restore_position(ParserState *parser) {
    if (!parser || !parser->position_saved) return;
    
    /* Restore lexer state */
    parser->lexer->buffer_pos = parser->saved_buffer_pos;
    parser->lexer->buffer_line = parser->saved_buffer_line;
    parser->lexer->buffer_column = parser->saved_buffer_column;
    parser->lexer->current_token = parser->saved_current_token;
    parser->lexer->token_value = parser->saved_token_value;
    parser->lexer->token_length = parser->saved_token_length;
    parser->position_saved = false;
    
    printf("DEBUG: Restored parser position to buffer %ld, token %d\n", 
           parser->saved_buffer_pos, parser->saved_current_token);
}

/* Address calculation functions */
void parser_initialize_address_space(ParserState *parser) {
    if (!parser) return;
    
    /* Initialize address space for symbols */
    parser->symbol_table.current_address = 0x1000;  /* Start at 4KB */
    parser->symbol_table.function_offset = 0x1000;  /* Functions start at 4KB */
    parser->symbol_table.variable_offset = 0x2000;  /* Variables start at 8KB */
    
    printf("DEBUG: Initialized address space - Functions: 0x%lx, Variables: 0x%lx\n",
           parser->symbol_table.function_offset, parser->symbol_table.variable_offset);
}

I64 parser_calculate_function_address(ParserState *parser, U8 *function_name) {
    if (!parser || !function_name) return 0;
    
    /* Find the function in the symbol table */
    ASTNode *func_node = parser_lookup_symbol(parser, function_name);
    if (!func_node || func_node->type != NODE_FUNCTION) {
        printf("ERROR: Function '%s' not found in symbol table\n", (char*)function_name);
        return 0;
    }
    
    /* Calculate address based on function position in symbol table */
    I64 function_index = -1;
    for (I64 i = 0; i < parser->symbol_table.count; i++) {
        if (parser->symbol_table.symbols[i] == func_node) {
            function_index = i;
            break;
        }
    }
    
    if (function_index == -1) {
        printf("ERROR: Function '%s' not found in symbol table indices\n", (char*)function_name);
        return 0;
    }
    
    /* Calculate function address: base + (index * function_size) */
    I64 function_size = 0x100;  /* Assume 256 bytes per function for now */
    I64 function_address = parser->symbol_table.function_offset + (function_index * function_size);
    
    printf("DEBUG: Function '%s' address: 0x%lx (index: %ld, size: 0x%lx)\n",
           (char*)function_name, function_address, function_index, function_size);
    
    return function_address;
}

I64 parser_calculate_variable_address(ParserState *parser, U8 *variable_name) {
    if (!parser || !variable_name) return 0;
    
    /* Find the variable in the symbol table */
    ASTNode *var_node = parser_lookup_symbol(parser, variable_name);
    if (!var_node || (var_node->type != NODE_VARIABLE && var_node->type != NODE_IDENTIFIER)) {
        printf("ERROR: Variable '%s' not found in symbol table\n", (char*)variable_name);
        return 0;
    }
    
    /* Calculate address based on variable position in symbol table */
    I64 variable_index = -1;
    for (I64 i = 0; i < parser->symbol_table.count; i++) {
        if (parser->symbol_table.symbols[i] == var_node) {
            variable_index = i;
            break;
        }
    }
    
    if (variable_index == -1) {
        printf("ERROR: Variable '%s' not found in symbol table indices\n", (char*)variable_name);
        return 0;
    }
    
    /* Calculate variable address: base + (index * variable_size) */
    I64 variable_size = 8;  /* Assume 8 bytes per variable (I64) */
    I64 variable_address = parser->symbol_table.variable_offset + (variable_index * variable_size);
    
    printf("DEBUG: Variable '%s' address: 0x%lx (index: %ld, size: %ld)\n",
           (char*)variable_name, variable_address, variable_index, variable_size);
    
    return variable_address;
}

I64 parser_calculate_relative_address(ParserState *parser, I64 from_address, I64 to_address) {
    if (!parser) return 0;
    
    /* Calculate relative address for CALL instruction */
    I64 relative_address = to_address - from_address - 5; /* -5 for CALL instruction size */
    
    printf("DEBUG: Relative address calculation: 0x%lx -> 0x%lx = %ld (0x%lx)\n",
           from_address, to_address, relative_address, relative_address);
    
    return relative_address;
}

Bool parser_is_assembly_token(TokenType token) {
    /* TODO: Implement assembly token checking */
    return false;
}

Bool parser_is_assembly_register_token(TokenType token) {
    /* TODO: Implement assembly register token checking */
    return false;
}

Bool parser_is_assembly_opcode_token(TokenType token) {
    /* TODO: Implement assembly opcode token checking */
    return false;
}

X86Register parser_get_assembly_register(TokenType token, U8 *name) {
    /* TODO: Implement assembly register parsing */
    return REG_NONE;
}

U8* parser_get_assembly_opcode(TokenType token, U8 *name) {
    /* TODO: Implement assembly opcode parsing */
    return NULL;
}

/*
 * Scope Management Implementation
 */

ScopeLevel* scope_level_new(ParserState *parser, Bool is_function_scope, Bool is_block_scope) {
    if (!parser) return NULL;
    
    ScopeLevel *scope = malloc(sizeof(ScopeLevel));
    if (!scope) return NULL;
    
    /* Initialize scope */
    scope->parent = NULL;
    scope->variable_count = 0;
    scope->variable_capacity = 16; /* Initial capacity */
    scope->scope_id = parser->scope_stack.current_scope_depth;
    scope->stack_offset = 0;
    scope->is_function_scope = is_function_scope;
    scope->is_block_scope = is_block_scope;
    
    /* Allocate variable array */
    scope->variables = (ASTNode**)calloc(scope->variable_capacity, sizeof(ASTNode*));
    if (!scope->variables) {
        free(scope);
        return NULL;
    }
    
    printf("DEBUG: Created scope level %lld (function=%d, block=%d)\n", 
           scope->scope_id, is_function_scope, is_block_scope);
    
    return scope;
}

void scope_level_free(ScopeLevel *scope) {
    if (!scope) return;
    
    printf("DEBUG: Freeing scope level %lld\n", scope->scope_id);
    
    /* Free variables array */
    if (scope->variables) {
        free(scope->variables);
    }
    
    free(scope);
}

Bool parser_enter_scope(ParserState *parser, Bool is_function_scope, Bool is_block_scope) {
    if (!parser) return false;
    
    /* Create new scope level */
    ScopeLevel *new_scope = scope_level_new(parser, is_function_scope, is_block_scope);
    if (!new_scope) return false;
    
    /* Set parent to current scope */
    if (parser->scope_stack.scope_count > 0) {
        new_scope->parent = parser->scope_stack.scopes[parser->scope_stack.scope_count - 1];
    }
    
    /* Expand scope stack if needed */
    if (parser->scope_stack.scope_count >= parser->scope_stack.scope_capacity) {
        I64 new_capacity = parser->scope_stack.scope_capacity * 2;
        ScopeLevel **new_scopes = (ScopeLevel**)realloc(parser->scope_stack.scopes, 
                                                        new_capacity * sizeof(ScopeLevel*));
        if (!new_scopes) {
            scope_level_free(new_scope);
            return false;
        }
        parser->scope_stack.scopes = new_scopes;
        parser->scope_stack.scope_capacity = new_capacity;
    }
    
    /* Add scope to stack */
    parser->scope_stack.scopes[parser->scope_stack.scope_count] = new_scope;
    parser->scope_stack.scope_count++;
    parser->scope_stack.current_scope_depth++;
    
    printf("DEBUG: Entered scope (depth=%lld, function=%d, block=%d)\n", 
           parser->scope_stack.current_scope_depth, is_function_scope, is_block_scope);
    
    return true;
}

Bool parser_exit_scope(ParserState *parser) {
    if (!parser || parser->scope_stack.scope_count == 0) return false;
    
    /* Get current scope */
    ScopeLevel *current_scope = parser->scope_stack.scopes[parser->scope_stack.scope_count - 1];
    
    printf("DEBUG: Exiting scope level %lld (variables=%lld)\n", 
           current_scope->scope_id, current_scope->variable_count);
    
    /* Free the scope */
    scope_level_free(current_scope);
    
    /* Remove from stack */
    parser->scope_stack.scope_count--;
    parser->scope_stack.current_scope_depth--;
    
    printf("DEBUG: Exited scope (depth=%lld)\n", parser->scope_stack.current_scope_depth);
    
    return true;
}

ScopeLevel* parser_get_current_scope(ParserState *parser) {
    if (!parser || parser->scope_stack.scope_count == 0) return NULL;
    return parser->scope_stack.scopes[parser->scope_stack.scope_count - 1];
}

Bool scope_add_variable(ScopeLevel *scope, ASTNode *variable) {
    if (!scope || !variable) return false;
    
    /* Check for variable name collision in current scope */
    if (scope_lookup_variable(scope, variable->data.identifier.name)) {
        printf("WARNING: Variable '%s' already defined in current scope\n", 
               variable->data.identifier.name);
        return false;
    }
    
    /* Expand variables array if needed */
    if (scope->variable_count >= scope->variable_capacity) {
        I64 new_capacity = scope->variable_capacity * 2;
        ASTNode **new_variables = (ASTNode**)realloc(scope->variables, 
                                                     new_capacity * sizeof(ASTNode*));
        if (!new_variables) return false;
        scope->variables = new_variables;
        scope->variable_capacity = new_capacity;
    }
    
    /* Add variable to scope */
    scope->variables[scope->variable_count] = variable;
    scope->variable_count++;
    
    /* Set stack offset for local variables */
    if (scope->is_function_scope || scope->is_block_scope) {
        variable->data.variable.stack_offset = scope->stack_offset;
        scope->stack_offset += 8; /* Assume 8-byte alignment for now */
    }
    
    printf("DEBUG: Added variable '%s' to scope %lld (stack_offset=%lld)\n", 
           variable->data.identifier.name, scope->scope_id, variable->data.variable.stack_offset);
    
    return true;
}

ASTNode* scope_lookup_variable(ScopeLevel *scope, U8 *name) {
    if (!scope || !name) return NULL;
    
    /* Search in current scope */
    for (I64 i = 0; i < scope->variable_count; i++) {
        if (scope->variables[i] && scope->variables[i]->data.identifier.name) {
            if (strcmp((char*)scope->variables[i]->data.identifier.name, (char*)name) == 0) {
                printf("DEBUG: Found variable '%s' in scope %lld\n", name, scope->scope_id);
                return scope->variables[i];
            }
        }
    }
    
    return NULL;
}

ASTNode* parser_lookup_variable_in_scope(ParserState *parser, U8 *name) {
    if (!parser || !name) return NULL;
    
    /* Search from current scope up to global scope */
    for (I64 i = parser->scope_stack.scope_count - 1; i >= 0; i--) {
        ScopeLevel *scope = parser->scope_stack.scopes[i];
        ASTNode *variable = scope_lookup_variable(scope, name);
        if (variable) {
            return variable;
        }
    }
    
    /* Also check the global symbol table */
    return parser_lookup_symbol(parser, name);
}

Bool parser_is_variable_defined_in_scope(ParserState *parser, U8 *name) {
    return parser_lookup_variable_in_scope(parser, name) != NULL;
}

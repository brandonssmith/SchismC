/*
 * Enhanced Parser Implementation for SchismC
 * Ported from TempleOS HolyC with assembly-influenced enhancements
 */

#include "parser.h"
#include "type_checker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Define TRUE and FALSE if not already defined */
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* Forward declarations */
ASTNode* parse_program(ParserState *parser);
ASTNode* parse_statement(ParserState *parser);
ASTNode* parse_assignment_or_expression_statement(ParserState *parser);
ASTNode* parse_simple_expression(ParserState *parser);
ASTNode* parse_range_comparison(ParserState *parser, ASTNode *first_expr);

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
    
    /* Initialize error recovery state */
    parser_init_recovery_state(parser);
    
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
    
    /* Initialize built-in functions */
    parser_initialize_builtin_functions(parser);
    
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

SchismTokenType parser_current_token(ParserState *parser) {
    if (!parser || !parser->lexer) return TK_EOF;
    return parser->lexer->current_token;
}

SchismTokenType parser_next_token(ParserState *parser) {
    if (!parser || !parser->lexer) return TK_EOF;
    printf("DEBUG: parser_next_token - calling lex_next_token\n");
    SchismTokenType token = lex_next_token(parser->lexer);
    printf("DEBUG: parser_next_token - got token: %d\n", token);
    return token;
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

Bool parser_match_token(ParserState *parser, SchismTokenType token) {
    if (parser_current_token(parser) == token) {
        parser_next_token(parser);
        return true;
    }
    return false;
}

SchismTokenType parser_expect_token(ParserState *parser, SchismTokenType expected) {
    SchismTokenType current = parser_current_token(parser);
    printf("DEBUG: parser_expect_token - expecting %d, current token: %d\n", expected, current);
    if (current == expected) {
        printf("DEBUG: parser_expect_token - token matches, calling parser_next_token\n");
        parser_next_token(parser);
        printf("DEBUG: parser_expect_token - parser_next_token completed\n");
        return current;
    }
    
    printf("DEBUG: parser_expect_token - token mismatch, calling parser_expected_error\n");
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
                "Parse error at line %I64d, column %I64d: %s", 
                line, column, message);
        printf("ERROR: %s\n", parser->last_error);
    }
    
    /* Attempt error recovery if not already in recovery mode */
    if (!parser_is_in_recovery_mode(parser) && parser_can_recover(parser)) {
        printf("Attempting error recovery...\n");
        
        /* Try different recovery strategies based on error context */
        Bool recovered = FALSE;
        
        /* Try to recover from syntax errors */
        if (strstr((char*)message, "Expected") || strstr((char*)message, "syntax")) {
            recovered = parser_recover_from_syntax_error(parser, (U8*)"syntax error");
        }
        
        /* Try to recover from missing tokens */
        if (!recovered && strstr((char*)message, "missing")) {
            recovered = parser_recover_from_missing_token(parser, ';');
        }
        
        /* Try to recover from unexpected tokens */
        if (!recovered && strstr((char*)message, "unexpected")) {
            recovered = parser_recover_from_unexpected_token(parser, parser_current_token(parser));
        }
        
        /* Default recovery: skip to semicolon */
        if (!recovered) {
            recovered = parser_recover_from_incomplete_statement(parser);
        }
        
        if (recovered) {
            printf("Error recovery successful, continuing parsing...\n");
        } else {
            printf("Error recovery failed, stopping parsing.\n");
        }
    }
}

void parser_expected_error(ParserState *parser, SchismTokenType expected, SchismTokenType found) {
    U8 error_msg[256];
    snprintf((char*)error_msg, sizeof(error_msg),
            "Expected token %d, but found token %d", expected, found);
    
    parser_error(parser, error_msg);
}

/*
 * Error recovery implementation
 */

/* Initialize error recovery state */
void parser_init_recovery_state(ParserState *parser) {
    if (!parser) return;
    
    parser->recovery_mode = FALSE;
    parser->recovery_depth = 0;
    parser->max_recovery_depth = 10;
    parser->recovery_attempts = 0;
    parser->max_recovery_attempts = 5;
    parser->recovery_successful = FALSE;
    
    /* Initialize recovery state */
    parser->recovery_state.saved_buffer_pos = 0;
    parser->recovery_state.saved_buffer_line = 0;
    parser->recovery_state.saved_buffer_column = 0;
    parser->recovery_state.saved_current_token = TK_EOF;
    parser->recovery_state.saved_token_value = NULL;
    parser->recovery_state.saved_token_length = 0;
    parser->recovery_state.saved_error_count = 0;
    parser->recovery_state.saved_warning_count = 0;
}

/* Set recovery mode */
void parser_set_recovery_mode(ParserState *parser, Bool enabled) {
    if (!parser) return;
    parser->recovery_mode = enabled;
}

/* Check if in recovery mode */
Bool parser_is_in_recovery_mode(ParserState *parser) {
    if (!parser) return FALSE;
    return parser->recovery_mode;
}

/* Check if recovery is possible */
Bool parser_can_recover(ParserState *parser) {
    if (!parser) return FALSE;
    return parser->recovery_depth < parser->max_recovery_depth &&
           parser->recovery_attempts < parser->max_recovery_attempts;
}

/* Save recovery state */
void parser_save_recovery_state(ParserState *parser) {
    if (!parser) return;
    
    parser->recovery_state.saved_buffer_pos = parser->lexer->buffer_pos;
    parser->recovery_state.saved_buffer_line = parser->lexer->token_line;
    parser->recovery_state.saved_buffer_column = parser->lexer->token_column;
    parser->recovery_state.saved_current_token = parser->lexer->current_token;
    parser->recovery_state.saved_token_value = parser->lexer->token_value ? strdup((char*)parser->lexer->token_value) : NULL;
    parser->recovery_state.saved_token_length = parser->lexer->token_length;
    parser->recovery_state.saved_error_count = parser->error_count;
    parser->recovery_state.saved_warning_count = parser->warning_count;
}

/* Restore recovery state */
void parser_restore_recovery_state(ParserState *parser) {
    if (!parser) return;
    
    parser->lexer->buffer_pos = parser->recovery_state.saved_buffer_pos;
    parser->lexer->token_line = parser->recovery_state.saved_buffer_line;
    parser->lexer->token_column = parser->recovery_state.saved_buffer_column;
    parser->lexer->current_token = parser->recovery_state.saved_current_token;
    if (parser->lexer->token_value) {
        free(parser->lexer->token_value);
    }
    parser->lexer->token_value = parser->recovery_state.saved_token_value;
    parser->lexer->token_length = parser->recovery_state.saved_token_length;
    parser->error_count = parser->recovery_state.saved_error_count;
    parser->warning_count = parser->recovery_state.saved_warning_count;
}

/* Skip to semicolon */
Bool parser_skip_to_semicolon(ParserState *parser) {
    if (!parser || !parser->lexer) return FALSE;
    
    I64 tokens_skipped = 0;
    I64 max_skip = 50; /* Prevent infinite loops */
    
    while (parser->lexer->current_token != ';' && 
           parser->lexer->current_token != TK_EOF && 
           tokens_skipped < max_skip) {
        lex_next_token(parser->lexer);
        tokens_skipped++;
    }
    
    if (parser->lexer->current_token == ';') {
        lex_next_token(parser->lexer); /* consume semicolon */
        return TRUE;
    }
    
    return FALSE;
}

/* Skip to matching brace */
Bool parser_skip_to_brace(ParserState *parser, SchismTokenType open_brace, SchismTokenType close_brace) {
    if (!parser || !parser->lexer) return FALSE;
    
    I64 brace_depth = 0;
    I64 tokens_skipped = 0;
    I64 max_skip = 200; /* Prevent infinite loops */
    
    while (tokens_skipped < max_skip) {
        if (parser->lexer->current_token == open_brace) {
            brace_depth++;
        } else if (parser->lexer->current_token == close_brace) {
            if (brace_depth == 0) {
                lex_next_token(parser->lexer); /* consume closing brace */
                return TRUE;
            }
            brace_depth--;
        } else if (parser->lexer->current_token == TK_EOF) {
            break;
        }
        
        lex_next_token(parser->lexer);
        tokens_skipped++;
    }
    
    return FALSE;
}

/* Skip to specific keyword */
Bool parser_skip_to_keyword(ParserState *parser, U8 *keyword) {
    if (!parser || !parser->lexer || !keyword) return FALSE;
    
    I64 tokens_skipped = 0;
    I64 max_skip = 100; /* Prevent infinite loops */
    
    while (tokens_skipped < max_skip) {
        if (parser->lexer->current_token == TK_IDENT && 
            parser->lexer->token_value &&
            strcmp((char*)parser->lexer->token_value, (char*)keyword) == 0) {
            return TRUE;
        }
        
        if (parser->lexer->current_token == TK_EOF) {
            break;
        }
        
        lex_next_token(parser->lexer);
        tokens_skipped++;
    }
    
    return FALSE;
}

/* Skip to newline */
Bool parser_skip_to_newline(ParserState *parser) {
    if (!parser || !parser->lexer) return FALSE;
    
    I64 tokens_skipped = 0;
    I64 max_skip = 50; /* Prevent infinite loops */
    
    while (tokens_skipped < max_skip) {
        if (parser->lexer->current_token == '\n' || 
            parser->lexer->current_token == TK_EOF) {
            if (parser->lexer->current_token == '\n') {
                lex_next_token(parser->lexer); /* consume newline */
            }
            return TRUE;
        }
        
        lex_next_token(parser->lexer);
        tokens_skipped++;
    }
    
    return FALSE;
}

/* Insert missing token (simulate by not consuming current token) */
Bool parser_insert_missing_token(ParserState *parser, SchismTokenType token) {
    if (!parser) return FALSE;
    
    /* For now, just report the insertion */
    U8 msg[256];
    snprintf((char*)msg, sizeof(msg), "Inserted missing token %d", token);
    parser_warning(parser, msg);
    
    return TRUE;
}

/* Delete current token */
Bool parser_delete_current_token(ParserState *parser) {
    if (!parser || !parser->lexer) return FALSE;
    
    U8 msg[256];
    snprintf((char*)msg, sizeof(msg), "Deleted unexpected token %d", parser->lexer->current_token);
    parser_warning(parser, msg);
    
    lex_next_token(parser->lexer);
    return TRUE;
}

/* Replace current token */
Bool parser_replace_current_token(ParserState *parser, SchismTokenType new_token) {
    if (!parser || !parser->lexer) return FALSE;
    
    U8 msg[256];
    snprintf((char*)msg, sizeof(msg), "Replaced token %d with %d", 
             parser->lexer->current_token, new_token);
    parser_warning(parser, msg);
    
    parser->lexer->current_token = new_token;
    return TRUE;
}

/* Restart statement */
Bool parser_restart_statement(ParserState *parser) {
    if (!parser) return FALSE;
    
    /* Skip to next statement boundary */
    return parser_skip_to_semicolon(parser);
}

/* Restart function */
Bool parser_restart_function(ParserState *parser) {
    if (!parser) return FALSE;
    
    /* Skip to next function or end of file */
    return parser_skip_to_keyword(parser, (U8*)"I64") || 
           parser_skip_to_keyword(parser, (U8*)"U64") ||
           parser_skip_to_keyword(parser, (U8*)"F64");
}

/* Restart block */
Bool parser_restart_block(ParserState *parser) {
    if (!parser) return FALSE;
    
    /* Skip to matching closing brace */
    return parser_skip_to_brace(parser, '{', '}');
}

/* Main error recovery function */
Bool parser_attempt_error_recovery(ParserState *parser, ErrorRecoveryInfo *recovery) {
    if (!parser || !recovery) return FALSE;
    
    if (!parser_can_recover(parser)) {
        return FALSE;
    }
    
    parser->recovery_attempts++;
    parser->recovery_depth++;
    parser->recovery_mode = TRUE;
    
    Bool success = FALSE;
    
    switch (recovery->strategy) {
        case RECOVERY_SKIP_TO_SEMICOLON:
            success = parser_skip_to_semicolon(parser);
            break;
            
        case RECOVERY_SKIP_TO_BRACE:
            success = parser_skip_to_brace(parser, '{', '}');
            break;
            
        case RECOVERY_SKIP_TO_PAREN:
            success = parser_skip_to_brace(parser, '(', ')');
            break;
            
        case RECOVERY_SKIP_TO_KEYWORD:
            success = parser_skip_to_keyword(parser, recovery->target_keyword);
            break;
            
        case RECOVERY_SKIP_TO_NEWLINE:
            success = parser_skip_to_newline(parser);
            break;
            
        case RECOVERY_INSERT_TOKEN:
            success = parser_insert_missing_token(parser, recovery->target_token);
            break;
            
        case RECOVERY_DELETE_TOKEN:
            success = parser_delete_current_token(parser);
            break;
            
        case RECOVERY_REPLACE_TOKEN:
            success = parser_replace_current_token(parser, recovery->target_token);
            break;
            
        case RECOVERY_RESTART_STATEMENT:
            success = parser_restart_statement(parser);
            break;
            
        case RECOVERY_RESTART_FUNCTION:
            success = parser_restart_function(parser);
            break;
            
        case RECOVERY_RESTART_BLOCK:
            success = parser_restart_block(parser);
            break;
            
        default:
            success = FALSE;
            break;
    }
    
    parser->recovery_successful = success;
    parser->recovery_depth--;
    
    if (success) {
        parser->recovery_mode = FALSE;
        parser->recovery_attempts = 0;
    }
    
    return success;
}

/* Context-aware error recovery functions */
Bool parser_recover_from_syntax_error(ParserState *parser, U8 *context) {
    if (!parser) return FALSE;
    
    ErrorRecoveryInfo recovery = {0};
    
    /* Choose recovery strategy based on context */
    if (strstr((char*)context, "statement")) {
        recovery.strategy = RECOVERY_SKIP_TO_SEMICOLON;
    } else if (strstr((char*)context, "function")) {
        recovery.strategy = RECOVERY_SKIP_TO_BRACE;
    } else if (strstr((char*)context, "expression")) {
        recovery.strategy = RECOVERY_SKIP_TO_SEMICOLON;
    } else {
        recovery.strategy = RECOVERY_SKIP_TO_NEWLINE;
    }
    
    return parser_attempt_error_recovery(parser, &recovery);
}

Bool parser_recover_from_missing_token(ParserState *parser, SchismTokenType expected) {
    if (!parser) return FALSE;
    
    ErrorRecoveryInfo recovery = {0};
    recovery.strategy = RECOVERY_INSERT_TOKEN;
    recovery.target_token = expected;
    
    return parser_attempt_error_recovery(parser, &recovery);
}

Bool parser_recover_from_unexpected_token(ParserState *parser, SchismTokenType unexpected) {
    if (!parser) return FALSE;
    
    ErrorRecoveryInfo recovery = {0};
    recovery.strategy = RECOVERY_DELETE_TOKEN;
    
    return parser_attempt_error_recovery(parser, &recovery);
}

Bool parser_recover_from_incomplete_statement(ParserState *parser) {
    if (!parser) return FALSE;
    
    ErrorRecoveryInfo recovery = {0};
    recovery.strategy = RECOVERY_SKIP_TO_SEMICOLON;
    
    return parser_attempt_error_recovery(parser, &recovery);
}

Bool parser_recover_from_incomplete_expression(ParserState *parser) {
    if (!parser) return FALSE;
    
    ErrorRecoveryInfo recovery = {0};
    recovery.strategy = RECOVERY_SKIP_TO_SEMICOLON;
    
    return parser_attempt_error_recovery(parser, &recovery);
}

Bool parser_recover_from_incomplete_function(ParserState *parser) {
    if (!parser) return FALSE;
    
    ErrorRecoveryInfo recovery = {0};
    recovery.strategy = RECOVERY_SKIP_TO_BRACE;
    
    return parser_attempt_error_recovery(parser, &recovery);
}

Bool parser_recover_from_incomplete_block(ParserState *parser) {
    if (!parser) return FALSE;
    
    ErrorRecoveryInfo recovery = {0};
    recovery.strategy = RECOVERY_SKIP_TO_BRACE;
    
    return parser_attempt_error_recovery(parser, &recovery);
}

/*
 * Main parsing functions
 */

ASTNode* parse_program(ParserState *parser) {
    printf("DEBUG: parse_program - starting\n");
    if (!parser) return NULL;
    
    /* Parse program */
    
    /* Create root program node */
    printf("DEBUG: parse_program - creating root program node\n");
    ASTNode *program = ast_node_new(NODE_PROGRAM, 1, 1);
    if (!program) return NULL;
    
    parser->root = program;
    parser->current_node = program;
    
    /* Parse statements until EOF */
    printf("DEBUG: parse_program - starting statement parsing loop\n");
    while (parser_current_token(parser) != TK_EOF) {
        printf("DEBUG: parse_program - current token: %d, parsing statement\n", parser_current_token(parser));
        ASTNode *stmt = parse_statement(parser);
        if (stmt) {
            printf("DEBUG: parse_program - statement parsed successfully, adding to program\n");
            ast_node_add_child(program, stmt);
        } else {
            printf("DEBUG: parse_program - statement parsing failed, skipping to next token\n");
            /* Skip to next statement on error */
            parser_next_token(parser);
        }
    }
    
    /* Program parsing complete */
    printf("DEBUG: parse_program - completed successfully\n");
    
    /* Perform type checking on the AST */
    printf("DEBUG: parse_program - performing type checking\n");
    if (!type_check_ast_node(program)) {
        printf("ERROR: Type checking failed\n");
        ast_node_free(program);
        return NULL;
    }
    printf("DEBUG: parse_program - type checking passed\n");
    
    return program;
}

ASTNode* parse_statement(ParserState *parser) {
    printf("DEBUG: parse_statement - starting, current token: %d\n", parser_current_token(parser));
    if (!parser) return NULL;
    
    /* Add token name for debugging */
    const char* token_name = "UNKNOWN";
    switch (parser_current_token(parser)) {
        case TK_RETURN: token_name = "TK_RETURN"; break;
        case TK_STR: token_name = "TK_STR"; break;
        case TK_I64: token_name = "TK_I64"; break;
        case TK_IDENT: token_name = "TK_IDENT"; break;
        case ';': token_name = "SEMICOLON"; break;
        case '}': token_name = "CLOSE_BRACE"; break;
        case TK_EOF: token_name = "TK_EOF"; break;
        default: token_name = "UNKNOWN"; break;
    }
    printf("DEBUG: parse_statement - token: %d (%s)\n", parser_current_token(parser), token_name);
    
    SchismTokenType current = parser_current_token(parser);
    
    /* Parse statement based on current token */
    
    switch (current) {
        case TK_IF:
            return parse_if_statement(parser);
        case TK_WHILE:
            return parse_while_statement(parser);
        case TK_DO:
            return parse_do_while_statement(parser);
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
        case TK_ASM:
            return parse_inline_assembly_block(parser);
        case TK_REG:
        case TK_NOREG:
            return parse_register_directive(parser);
        case TK_TRY:
            return parse_try_block(parser);
        case TK_THROW:
            return parse_throw_statement(parser);
        case TK_CLASS:
        case TK_UNION:
        case TK_PUBLIC:
            /* Check if this is a type-prefixed union (public I64i union I64) */
            if (current == TK_PUBLIC || current == TK_IDENT) {
                /* Save position and check for type-prefixed union pattern */
                parser_save_position(parser);
                Bool is_type_prefixed = false;
                
                if (current == TK_PUBLIC) {
                    parser_next_token(parser); /* consume 'public' */
                }
                
                /* Check if next token is an identifier (prefix type) */
                if (parser_current_token(parser) == TK_IDENT) {
                    parser_next_token(parser); /* consume prefix type */
                    
                    /* Check if next token is 'union' */
                    if (parser_current_token(parser) == TK_UNION) {
                        is_type_prefixed = true;
                    }
                }
                
                /* Restore position */
                parser_restore_position(parser);
                
                if (is_type_prefixed) {
                    return parse_type_prefixed_union(parser);
                }
            }
            return parse_class_definition(parser);
        case TK_IDENT:
            /* Check if this is a label (identifier followed by ':' or '::') */
            {
                /* Save current position and token */
                I64 saved_buffer_pos = parser->lexer->buffer_pos;
                SchismTokenType saved_token = parser->lexer->current_token;
                printf("DEBUG: TK_IDENT case - saved buffer pos: %lld, current token: %d\n", saved_buffer_pos, parser_current_token(parser));
                
                /* Look ahead to see if next token is ':' or '::' */
                parser_next_token(parser);
                printf("DEBUG: TK_IDENT case - after lookahead, current token: %d\n", parser_current_token(parser));
                if (parser_current_token(parser) == ':' || parser_current_token(parser) == TK_DBL_COLON) {
                    /* Restore position and parse as label */
                    printf("DEBUG: TK_IDENT case - parsing as label\n");
                    parser->lexer->buffer_pos = saved_buffer_pos;
                    parser->lexer->current_token = saved_token;
                    return parse_label_statement(parser);
                } else {
                    /* Restore position and parse as variable/expression */
                    printf("DEBUG: TK_IDENT case - restoring position to %lld, parsing as assignment/expression\n", saved_buffer_pos);
                    parser->lexer->buffer_pos = saved_buffer_pos;
                    parser->lexer->current_token = saved_token;
                    printf("DEBUG: TK_IDENT case - after restore, current token: %d\n", parser_current_token(parser));
                    return parse_assignment_or_expression_statement(parser);
                }
            }
        case '{':
            return parse_block_statement(parser);
        case TK_STR:
            printf("DEBUG: parse_statement - found TK_STR, calling parse_expression_statement directly\n");
            /* For string literals, bypass the assignment parsing and go directly to expression parsing */
            /* This avoids the problematic code path that causes hanging */
            return parse_expression_statement(parser);
        case TK_CHAR_CONST:
            /* Try to parse as assignment statement first */
            return parse_assignment_or_expression_statement(parser);
        case TK_TYPE_I0:
        case TK_TYPE_I8:
        case TK_TYPE_I16:
        case TK_TYPE_I32:
        case TK_TYPE_I64:
        case TK_TYPE_U0:
        case TK_TYPE_U8:
        case TK_TYPE_U16:
        case TK_TYPE_U32:
        case TK_TYPE_U64:
        case TK_TYPE_F32:
        case TK_TYPE_F64:
        case TK_TYPE_BOOL:
        case TK_TYPE_STRING:
            printf("DEBUG: parse_statement - found type token %d, checking if function or variable\n", current);
            /* Check if this is a function or variable declaration */
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
        case TK_AUTO:
            printf("DEBUG: parse_statement - found TK_AUTO, calling parse_type_inference\n");
            return parse_type_inference(parser);
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
    printf("DEBUG: parse_assignment_or_expression_statement - starting, current token: %d\n", parser_current_token(parser));
    if (!parser) return NULL;
    
    /* Parse identifier */
    if (parser_current_token(parser) != TK_IDENT) {
        printf("DEBUG: parse_assignment_or_expression_statement - not TK_IDENT, calling parse_expression_statement\n");
        return parse_expression_statement(parser);
    }
    
    /* Get the identifier name */
    U8 *var_name = parser_current_token_value(parser);
    
    /* Look ahead to see what comes after the identifier */
    parser_next_token(parser); /* Consume identifier */
    SchismTokenType op = parser_current_token(parser);
    
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
        
        /* Parse right side - use full expression parser */
        ASTNode *right_expr = parse_expression(parser);
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
        /* This could be a comma expression or other expression */
        /* We've already consumed the identifier, so create an identifier node */
        ASTNode *ident_node = ast_node_new(NODE_IDENTIFIER, parser_current_line(parser), parser_current_column(parser));
        if (!ident_node) return NULL;
        
        /* Set identifier name */
        if (var_name) {
            I64 len = strlen((char*)var_name);
            ident_node->data.identifier.name = (U8*)malloc(len + 1);
            if (ident_node->data.identifier.name) {
                strcpy((char*)ident_node->data.identifier.name, (char*)var_name);
            }
        }
        
        /* Check if this is a comma expression */
        if (parser_current_token(parser) == ',') {
            printf("DEBUG: parse_assignment_or_expression_statement - found comma, creating comma expression\n");
            
            /* This is a comma expression */
            parser_next_token(parser); /* Consume comma */
            
            /* Parse the right side */
            ASTNode *right = parse_comma_expression(parser);
            if (!right) {
                ast_node_free(ident_node);
                return NULL;
            }
            
            /* Create comma expression node */
            ASTNode *comma_node = ast_node_new(NODE_BINARY_OP, parser_current_line(parser), parser_current_column(parser));
            if (!comma_node) {
                ast_node_free(ident_node);
                ast_node_free(right);
                return NULL;
            }
            
            comma_node->data.binary_op.left = ident_node;
            comma_node->data.binary_op.right = right;
            comma_node->data.binary_op.op = BINOP_COMMA;
            
            /* Expect semicolon */
            parser_expect_token(parser, ';');
            
            return comma_node;
        } else {
            /* Just a single identifier expression */
            /* Expect semicolon */
            parser_expect_token(parser, ';');
            return ident_node;
        }
    }
    
    return NULL; /* Should never reach here */
}

ASTNode* parse_expression_statement(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: parse_expression_statement - starting\n");
    ASTNode *expr = parse_expression(parser);
    if (!expr) {
        printf("DEBUG: parse_expression_statement - failed to parse expression\n");
        return NULL;
    }
    
    printf("DEBUG: parse_expression_statement - parsed expression type %d\n", expr->type);
    
    /* Expect semicolon */
    parser_expect_token(parser, ';');
    
    printf("DEBUG: parse_expression_statement - completed successfully\n");
    return expr;
}

ASTNode* parse_expression(ParserState *parser) {
    if (!parser) return NULL;
    
    /* Parse comma expressions (lowest precedence) */
    return parse_comma_expression(parser);
}

ASTNode* parse_additive_expression(ParserState *parser) {
    if (!parser) return NULL;
    
    /* Parse multiplicative expressions first */
    ASTNode *left = parse_multiplicative_expression(parser);
    if (!left) return NULL;
    
    /* Parse additive operators (+, -) */
    while (parser_current_token(parser) == '+' || parser_current_token(parser) == '-') {
        SchismTokenType op = parser_current_token(parser);
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
        SchismTokenType op = parser_current_token(parser);
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
    
    SchismTokenType current = parser_current_token(parser);
    I64 line = parser_current_line(parser);
    I64 column = parser_current_column(parser);
    
    switch (current) {
        case TK_STR: {
            /* String literal */
            printf("DEBUG: parse_primary_expression - parsing string literal\n");
            ASTNode *node = ast_node_new(NODE_STRING, line, column);
            if (!node) {
                printf("DEBUG: parse_primary_expression - failed to create string node\n");
                return NULL;
            }
            
            U8 *value = parser_current_token_value(parser);
            if (value) {
                printf("DEBUG: parse_primary_expression - string value: %s\n", value);
                I64 len = strlen((char*)value);
                node->data.literal.str_value = (U8*)malloc(len + 1);
                if (node->data.literal.str_value) {
                    strcpy((char*)node->data.literal.str_value, (char*)value);
                }
            } else {
                printf("DEBUG: parse_primary_expression - no string value\n");
            }
            
            parser_next_token(parser);
            printf("DEBUG: parse_primary_expression - string literal parsed successfully\n");
            return node;
        }
        
        case TK_TRUE:
        case TK_FALSE: {
            /* Boolean literal */
            Bool bool_value = (current == TK_TRUE);
            parser_next_token(parser);
            
            ASTNode *node = ast_node_new(NODE_BOOLEAN, line, column);
            if (!node) return NULL;
            
            node->data.boolean.value = bool_value;
            printf("DEBUG: parse_primary_expression - boolean literal parsed: %s\n", bool_value ? "true" : "false");
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
            /* Character constant - check if it's multi-character */
            U8 *value = parser_current_token_value(parser);
            if (value && strlen((char*)value) > 1) {
                /* Multi-character constant */
                printf("DEBUG: Found multi-character constant: %s\n", value);
                ASTNode *node = ast_node_new(NODE_MULTI_CHAR_CONST, line, column);
                if (!node) return NULL;
                
                node->data.multi_char_const.value = value;
                node->data.multi_char_const.length = strlen((char*)value);
                
                /* Calculate integer value from character sequence */
                I64 int_value = 0;
                for (I64 i = 0; i < node->data.multi_char_const.length; i++) {
                    int_value = (int_value << 8) | value[i];
                }
                node->data.multi_char_const.int_value = int_value;
                
                parser_next_token(parser);
                return node;
            } else {
                /* Single character constant */
                ASTNode *node = ast_node_new(NODE_CHAR, line, column);
                if (!node) return NULL;
                
                if (value && strlen((char*)value) > 0) {
                    node->data.literal.char_value = value[0];
                }
                
                parser_next_token(parser);
                return node;
            }
        }
        
        case TK_IDENT: {
            /* Identifier - could be variable or function call */
            U8 *name = parser_current_token_value(parser);
            parser_next_token(parser);
            
            /* Check if this is a function call */
            if (parser_current_token(parser) == '(') {
                /* Function call with parentheses */
                return parse_function_call(parser, name, line, column);
            } else {
                /* Check if this is a function call without parentheses */
                /* In HolyC, function calls without parentheses are allowed */
                if (parser_is_function_defined_in_scope(parser, name)) {
                    printf("DEBUG: Found function call without parentheses: %s\n", name);
                    ASTNode *func_call_node = ast_node_new(NODE_FUNC_CALL_NO_PARENS, line, column);
                    if (func_call_node) {
                        func_call_node->data.func_call_no_parens.name = name;
                        func_call_node->data.func_call_no_parens.arguments = NULL; /* No arguments */
                        func_call_node->data.func_call_no_parens.return_type = (U8*)"I64"; /* Default return type */
                        return func_call_node;
                    }
                }
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
                
                /* Check for sub-int access pattern (identifier.type[index]) FIRST */
                if (parser_current_token(parser) == '.' && is_sub_int_access_pattern(parser)) {
                    ASTNode *sub_int_access = parse_sub_int_access(parser);
                    if (!sub_int_access) {
                        ast_node_free(node);
                        return NULL;
                    }
                    
                    /* Set the base object for sub-int access */
                    sub_int_access->data.sub_int_access.base_object = node;
                    node = sub_int_access;
                }
                /* Check for member access */
                else while (parser_current_token(parser) == '.' || parser_current_token(parser) == TK_DEREFERENCE) {
                    ASTNode *member_access = parse_member_access(parser);
                    if (!member_access) {
                        ast_node_free(node);
                        return NULL;
                    }
                    
                    /* Set the object for member access */
                    member_access->data.member_access.object = node;
                    node = member_access;
                }
                
                /* Check for array access AFTER member access */
                while (parser_current_token(parser) == '[') {
                    ASTNode *array_access = parse_array_access(parser);
                    if (!array_access) {
                        ast_node_free(node);
                        return NULL;
                    }
                    
                    /* Replace node with array_access */
                    array_access->data.array_access.array = node;
                    node = array_access;
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
        
        case '{': {
            /* Array initializer */
            return parse_array_initializer(parser);
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

ASTNode* parse_do_while_statement(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: Parsing do-while statement, current token: %d\n", parser_current_token(parser));
    
    I64 line = parser_current_line(parser);
    I64 column = parser_current_column(parser);
    
    /* Expect 'do' keyword */
    if (parser_current_token(parser) != TK_DO) {
        parser_error(parser, (U8*)"Expected 'do' keyword");
        return NULL;
    }
    parser_next_token(parser); /* consume 'do' */
    
    /* Parse body statement */
    ASTNode *body = parse_statement(parser);
    if (!body) {
        parser_error(parser, (U8*)"Expected statement after 'do'");
        return NULL;
    }
    
    /* Expect 'while' keyword */
    if (parser_current_token(parser) != TK_WHILE) {
        parser_error(parser, (U8*)"Expected 'while' after do statement");
        ast_node_free(body);
        return NULL;
    }
    parser_next_token(parser); /* consume 'while' */
    
    /* Expect '(' */
    if (parser_current_token(parser) != '(') {
        parser_error(parser, (U8*)"Expected '(' after 'while'");
        ast_node_free(body);
        return NULL;
    }
    parser_next_token(parser); /* consume '(' */
    
    /* Parse condition expression */
    ASTNode *condition = parse_expression(parser);
    if (!condition) {
        parser_error(parser, (U8*)"Expected condition expression in do-while statement");
        ast_node_free(body);
        return NULL;
    }
    
    /* Expect ')' */
    if (parser_current_token(parser) != ')') {
        parser_error(parser, (U8*)"Expected ')' after do-while condition");
        ast_node_free(body);
        ast_node_free(condition);
        return NULL;
    }
    parser_next_token(parser); /* consume ')' */
    
    /* Expect semicolon */
    if (parser_current_token(parser) != ';') {
        parser_error(parser, (U8*)"Expected ';' after do-while statement");
        ast_node_free(body);
        ast_node_free(condition);
        return NULL;
    }
    parser_next_token(parser); /* consume ';' */
    
    /* Create do-while statement node */
    ASTNode *do_while_node = ast_node_new(NODE_DO_WHILE_STMT, line, column);
    if (!do_while_node) {
        ast_node_free(body);
        ast_node_free(condition);
        return NULL;
    }
    
    /* Initialize do-while statement data */
    do_while_node->data.do_while_stmt.body = body;
    do_while_node->data.do_while_stmt.condition = condition;
    
    printf("DEBUG: Do-while statement parsed successfully\n");
    return do_while_node;
}

ASTNode* parse_for_statement(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: Parsing for statement, current token: %d\n", parser_current_token(parser));
    
    I64 line = parser_current_line(parser);
    I64 column = parser_current_column(parser);
    
    /* Expect 'for' keyword */
    if (parser_current_token(parser) != TK_FOR) {
        parser_error(parser, (U8*)"Expected 'for' keyword");
    return NULL;
    }
    parser_next_token(parser); /* consume 'for' */
    
    /* Expect '(' */
    if (parser_current_token(parser) != '(') {
        parser_error(parser, (U8*)"Expected '(' after 'for'");
        return NULL;
    }
    parser_next_token(parser); /* consume '(' */
    
    /* Parse initialization (optional) */
    ASTNode *init = NULL;
    if (parser_current_token(parser) != ';') {
        init = parse_statement(parser);
        if (!init) {
            parser_error(parser, (U8*)"Failed to parse for loop initialization");
            return NULL;
        }
    }
    
    /* Expect ';' after initialization */
    if (parser_current_token(parser) != ';') {
        parser_error(parser, (U8*)"Expected ';' after for loop initialization");
        if (init) ast_node_free(init);
        return NULL;
    }
    parser_next_token(parser); /* consume ';' */
    
    /* Parse condition (optional) */
    ASTNode *condition = NULL;
    if (parser_current_token(parser) != ';') {
        condition = parse_expression(parser);
        if (!condition) {
            parser_error(parser, (U8*)"Failed to parse for loop condition");
            if (init) ast_node_free(init);
            return NULL;
        }
    }
    
    /* Expect ';' after condition */
    if (parser_current_token(parser) != ';') {
        parser_error(parser, (U8*)"Expected ';' after for loop condition");
        if (init) ast_node_free(init);
        if (condition) ast_node_free(condition);
        return NULL;
    }
    parser_next_token(parser); /* consume ';' */
    
    /* Parse increment (optional) */
    ASTNode *increment = NULL;
    if (parser_current_token(parser) != ')') {
        increment = parse_expression(parser);
        if (!increment) {
            parser_error(parser, (U8*)"Failed to parse for loop increment");
            if (init) ast_node_free(init);
            if (condition) ast_node_free(condition);
            return NULL;
        }
    }
    
    /* Expect ')' */
    if (parser_current_token(parser) != ')') {
        parser_error(parser, (U8*)"Expected ')' after for loop increment");
        if (init) ast_node_free(init);
        if (condition) ast_node_free(condition);
        if (increment) ast_node_free(increment);
        return NULL;
    }
    parser_next_token(parser); /* consume ')' */
    
    /* Parse body statement */
    ASTNode *body = parse_statement(parser);
    if (!body) {
        parser_error(parser, (U8*)"Expected statement after for loop");
        if (init) ast_node_free(init);
        if (condition) ast_node_free(condition);
        if (increment) ast_node_free(increment);
        return NULL;
    }
    
    /* Create for statement node */
    ASTNode *for_node = ast_node_new(NODE_FOR_STMT, line, column);
    if (!for_node) {
        if (init) ast_node_free(init);
        if (condition) ast_node_free(condition);
        if (increment) ast_node_free(increment);
        ast_node_free(body);
        return NULL;
    }
    
    /* Initialize for statement data */
    for_node->data.for_stmt.init = init;
    for_node->data.for_stmt.condition = condition;
    for_node->data.for_stmt.increment = increment;
    for_node->data.for_stmt.body = body;
    
    printf("DEBUG: For statement parsed successfully\n");
    return for_node;
}

ASTNode* parse_switch_statement(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: Parsing switch statement, current token: %d\n", parser_current_token(parser));
    
    I64 line = parser_current_line(parser);
    I64 column = parser_current_column(parser);
    
    /* Expect 'switch' keyword */
    if (parser_current_token(parser) != TK_SWITCH) {
        parser_error(parser, (U8*)"Expected 'switch' keyword");
    return NULL;
    }
    parser_next_token(parser); /* consume 'switch' */
    
    /* Check for nobounds switch (switch [expr]) */
    Bool nobounds = false;
    if (parser_current_token(parser) == '[') {
        nobounds = true;
        parser_next_token(parser); /* consume '[' */
    }
    
    /* Expect '(' or '[' */
    if (parser_current_token(parser) != '(' && parser_current_token(parser) != '[') {
        parser_error(parser, (U8*)"Expected '(' or '[' after 'switch'");
        return NULL;
    }
    parser_next_token(parser); /* consume '(' or '[' */
    
    /* Parse switch expression */
    ASTNode *expression = parse_expression(parser);
    if (!expression) {
        parser_error(parser, (U8*)"Expected expression in switch statement");
        return NULL;
    }
    
    /* Expect ')' or ']' */
    if (parser_current_token(parser) != ')' && parser_current_token(parser) != ']') {
        parser_error(parser, (U8*)"Expected ')' or ']' after switch expression");
        ast_node_free(expression);
        return NULL;
    }
    parser_next_token(parser); /* consume ')' or ']' */
    
    /* Expect '{' */
    if (parser_current_token(parser) != '{') {
        parser_error(parser, (U8*)"Expected '{' after switch statement");
        ast_node_free(expression);
        return NULL;
    }
    parser_next_token(parser); /* consume '{' */
    
    /* Parse case statements */
    ASTNode *cases = NULL;
    ASTNode *default_case = NULL;
    ASTNode *last_case = NULL;
    
    while (parser_current_token(parser) != '}' && parser_current_token(parser) != TK_EOF) {
        if (parser_current_token(parser) == TK_CASE) {
            ASTNode *case_stmt = parse_case_statement(parser);
            if (!case_stmt) {
                parser_error(parser, (U8*)"Failed to parse case statement");
                if (cases) ast_node_free(cases);
                if (default_case) ast_node_free(default_case);
                ast_node_free(expression);
                return NULL;
            }
            
            /* Add to cases list */
            if (!cases) {
                cases = case_stmt;
                last_case = case_stmt;
            } else {
                last_case->next = case_stmt;
                last_case = case_stmt;
            }
        } else if (parser_current_token(parser) == TK_DEFAULT) {
            if (default_case) {
                parser_error(parser, (U8*)"Multiple default cases in switch statement");
                if (cases) ast_node_free(cases);
                if (default_case) ast_node_free(default_case);
                ast_node_free(expression);
                return NULL;
            }
            
            default_case = parse_default_statement(parser);
            if (!default_case) {
                parser_error(parser, (U8*)"Failed to parse default statement");
                if (cases) ast_node_free(cases);
                ast_node_free(expression);
                return NULL;
            }
        } else if (parser_current_token(parser) == TK_START) {
            /* Parse start block for sub-switch */
            ASTNode *start_block = parse_start_end_block(parser, true);
            if (!start_block) {
                parser_error(parser, (U8*)"Failed to parse start block");
                if (cases) ast_node_free(cases);
                if (default_case) ast_node_free(default_case);
                ast_node_free(expression);
                return NULL;
            }
            
            /* Add start block to cases list */
            if (!cases) {
                cases = start_block;
                last_case = start_block;
            } else {
                last_case->next = start_block;
                last_case = start_block;
            }
        } else if (parser_current_token(parser) == TK_END) {
            /* Parse end block for sub-switch */
            ASTNode *end_block = parse_start_end_block(parser, false);
            if (!end_block) {
                parser_error(parser, (U8*)"Failed to parse end block");
                if (cases) ast_node_free(cases);
                if (default_case) ast_node_free(default_case);
                ast_node_free(expression);
                return NULL;
            }
            
            /* Add end block to cases list */
            if (!cases) {
                cases = end_block;
                last_case = end_block;
            } else {
                last_case->next = end_block;
                last_case = end_block;
            }
        } else {
            parser_error(parser, (U8*)"Expected 'case', 'default', 'start', or 'end' in switch statement");
            if (cases) ast_node_free(cases);
            if (default_case) ast_node_free(default_case);
            ast_node_free(expression);
            return NULL;
        }
    }
    
    /* Expect '}' */
    if (parser_current_token(parser) != '}') {
        parser_error(parser, (U8*)"Expected '}' to close switch statement");
        if (cases) ast_node_free(cases);
        if (default_case) ast_node_free(default_case);
        ast_node_free(expression);
        return NULL;
    }
    parser_next_token(parser); /* consume '}' */
    
    /* Create switch statement node */
    ASTNode *switch_node = ast_node_new(NODE_SWITCH, line, column);
    if (!switch_node) {
        if (cases) ast_node_free(cases);
        if (default_case) ast_node_free(default_case);
        ast_node_free(expression);
        return NULL;
    }
    
    /* Initialize switch statement data */
    switch_node->data.switch_stmt.expression = expression;
    switch_node->data.switch_stmt.cases = cases;
    switch_node->data.switch_stmt.default_case = default_case;
    switch_node->data.switch_stmt.nobounds = nobounds;
    
    printf("DEBUG: Switch statement parsed successfully\n");
    return switch_node;
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
    if (!parser) return NULL;
    
    printf("DEBUG: Parsing break statement, current token: %d\n", parser_current_token(parser));
    
    I64 line = parser_current_line(parser);
    I64 column = parser_current_column(parser);
    
    /* Expect 'break' keyword */
    if (parser_current_token(parser) != TK_BREAK) {
        parser_error(parser, (U8*)"Expected 'break' keyword");
    return NULL;
    }
    parser_next_token(parser); /* consume 'break' */
    
    /* Expect semicolon */
    if (parser_current_token(parser) != ';') {
        parser_error(parser, (U8*)"Expected ';' after break statement");
        return NULL;
    }
    parser_next_token(parser); /* consume ';' */
    
    /* Create break statement node */
    ASTNode *break_node = ast_node_new(NODE_BREAK, line, column);
    if (!break_node) {
        return NULL;
    }
    
    printf("DEBUG: Break statement parsed successfully\n");
    return break_node;
}

ASTNode* parse_continue_statement(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: Parsing continue statement, current token: %d\n", parser_current_token(parser));
    
    I64 line = parser_current_line(parser);
    I64 column = parser_current_column(parser);
    
    /* Expect 'continue' keyword */
    if (parser_current_token(parser) != TK_CONTINUE) {
        parser_error(parser, (U8*)"Expected 'continue' keyword");
    return NULL;
    }
    parser_next_token(parser); /* consume 'continue' */
    
    /* Expect semicolon */
    if (parser_current_token(parser) != ';') {
        parser_error(parser, (U8*)"Expected ';' after continue statement");
        return NULL;
    }
    parser_next_token(parser); /* consume ';' */
    
    /* Create continue statement node */
    ASTNode *continue_node = ast_node_new(NODE_CONTINUE, line, column);
    if (!continue_node) {
        return NULL;
    }
    
    printf("DEBUG: Continue statement parsed successfully\n");
    return continue_node;
}

ASTNode* parse_goto_statement(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: Parsing goto statement, current token: %d\n", parser_current_token(parser));
    
    I64 line = parser_current_line(parser);
    I64 column = parser_current_column(parser);
    
    /* Expect 'goto' keyword */
    if (parser_current_token(parser) != TK_GOTO) {
        parser_error(parser, (U8*)"Expected 'goto' keyword");
    return NULL;
    }
    parser_next_token(parser); /* consume 'goto' */
    
    /* Parse label name */
    if (parser_current_token(parser) != TK_IDENT) {
        parser_error(parser, (U8*)"Expected label name after 'goto'");
        return NULL;
    }
    
    U8 *label_name = parser_current_token_value(parser);
    if (!label_name) {
        parser_error(parser, (U8*)"Failed to get label name");
        return NULL;
    }
    parser_next_token(parser); /* consume label name */
    
    /* Expect semicolon */
    if (parser_current_token(parser) != ';') {
        parser_error(parser, (U8*)"Expected ';' after goto statement");
        return NULL;
    }
    parser_next_token(parser); /* consume ';' */
    
    /* Create goto statement node */
    ASTNode *goto_node = ast_node_new(NODE_GOTO, line, column);
    if (!goto_node) {
        return NULL;
    }
    
    /* Initialize goto statement data */
    goto_node->data.goto_stmt.label_name = label_name;
    goto_node->data.goto_stmt.jump_target = 0; /* Will be set during codegen */
    
    printf("DEBUG: Goto statement parsed successfully, target: %s\n", label_name);
    return goto_node;
}

ASTNode* parse_block_statement(ParserState *parser) {
    printf("DEBUG: parse_block_statement - starting\n");
    if (!parser) return NULL;
    
    /* Expect opening brace */
    printf("DEBUG: parse_block_statement - expecting opening brace, current token: %d\n", parser_current_token(parser));
    if (parser_current_token(parser) != '{') {
        printf("DEBUG: parse_block_statement - failed to find opening brace\n");
        parser_error(parser, (U8*)"Expected '{' to start block");
        return NULL;
    }
    printf("DEBUG: parse_block_statement - found opening brace, consuming it\n");
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
    printf("DEBUG: parse_block_statement - starting statement parsing loop\n");
    int statement_count = 0;
    int max_statements = 100; /* Prevent infinite loops */
    while (parser_current_token(parser) != '}' && statement_count < max_statements) {
        statement_count++;
        printf("DEBUG: parse_block_statement - statement %d, current token: %d, parsing statement\n", statement_count, parser_current_token(parser));
        /* Check for end of file (should not happen in valid code) */
        if (parser_current_token(parser) == TK_EOF) {
            printf("DEBUG: parse_block_statement - unexpected EOF\n");
            parser_error(parser, (U8*)"Expected '}' to close block");
            if (entered_block_scope) {
                parser_exit_scope(parser);
            }
            ast_node_free(block_node);
            return NULL;
        }
        
        /* Parse a statement */
        printf("DEBUG: parse_block_statement - calling parse_statement for statement %d\n", statement_count);
        ASTNode *stmt = parse_statement(parser);
        printf("DEBUG: parse_block_statement - parse_statement returned for statement %d\n", statement_count);
        if (!stmt) {
            printf("DEBUG: parse_block_statement - statement parsing failed, skipping\n");
            /* If statement parsing fails, we might still have a valid block with other statements */
            /* Skip to next token and continue */
            parser_next_token(parser);
            continue;
        }
        
        /* Special handling for string literal + return statement combination */
        /* This is a workaround for the specific issue where string literals followed by return statements cause hangs */
        if (stmt->type == NODE_STRING && statement_count == 1) {
            printf("DEBUG: parse_block_statement - detected string literal as first statement, checking for potential return statement\n");
            /* Check if the next token is a return statement */
            SchismTokenType next_token = parser_current_token(parser);
            if (next_token == TK_RETURN) {
                printf("DEBUG: parse_block_statement - found return statement after string literal, applying workaround\n");
                /* Force token advancement to ensure proper state */
                parser_next_token(parser);
            }
        }
        
        /* Check for infinite loop condition */
        if (statement_count >= max_statements) {
            printf("DEBUG: parse_block_statement - maximum statements reached, breaking loop\n");
            parser_error(parser, (U8*)"Too many statements in block (possible infinite loop)");
            break;
        }
        printf("DEBUG: parse_block_statement - statement parsed successfully, type: %d\n", stmt->type);
        
        /* Add statement to block */
        ast_node_add_child(block_node, stmt);
        
        /* Set the block's statements field to point to the children list */
        /* This ensures consistency between the parent-child relationship and block statements */
        block_node->data.block.statements = block_node->children;
        block_node->data.block.statement_count++;
    }
    
    /* Consume the closing brace */
    printf("DEBUG: parse_block_statement - found closing brace, consuming it\n");
    parser_next_token(parser);
    
    /* Exit block scope if we entered one */
    if (entered_block_scope) {
        printf("DEBUG: parse_block_statement - exiting block scope\n");
        parser_exit_scope(parser);
    }
    
    printf("DEBUG: parse_block_statement - completed successfully\n");
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
    call_node->data.call.return_reg = X86_REG_NONE;
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
    
    /* Check for array declaration */
    if (parser_current_token(parser) == '[') {
        parser_next_token(parser); /* consume '[' */
        
        /* Parse array size */
        ASTNode *size_expr = NULL;
        if (parser_current_token(parser) != ']') {
            size_expr = parse_expression(parser);
            if (!size_expr) {
                parser_error(parser, (U8*)"Expected array size expression");
                ast_node_free(type_node);
                ast_node_free(var_node);
                return NULL;
            }
        }
        
        /* Expect ']' */
        if (parser_current_token(parser) != ']') {
            parser_error(parser, (U8*)"Expected ']' after array size");
            ast_node_free(type_node);
            ast_node_free(var_node);
            if (size_expr) ast_node_free(size_expr);
            return NULL;
        }
        parser_next_token(parser); /* consume ']' */
        
        /* Mark variable as array */
        var_node->data.identifier.is_array = true;
        var_node->data.identifier.array_size = size_expr;
    } else {
        var_node->data.identifier.is_array = false;
        var_node->data.identifier.array_size = NULL;
    }
    
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
            
            /* Right side: expression - parse full expression, not just primary */
            assign_node->data.assignment.right = parse_expression(parser);
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
ASTNode* parse_comma_expression(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: parse_comma_expression - starting, current token: %d\n", parser_current_token(parser));
    
    /* Parse assignment expressions first */
    ASTNode *left = parse_assignment_expression(parser);
    if (!left) {
        printf("DEBUG: parse_comma_expression - failed to parse left side\n");
        return NULL;
    }
    
    printf("DEBUG: parse_comma_expression - parsed left side, current token: %d\n", parser_current_token(parser));
    
    /* Check for comma operator */
    if (parser_current_token(parser) == ',') {
        printf("DEBUG: parse_comma_expression - found comma operator\n");
        parser_next_token(parser); /* Consume comma */
        
        /* Parse right side */
        ASTNode *right = parse_comma_expression(parser);
        if (!right) {
            printf("DEBUG: parse_comma_expression - failed to parse right side\n");
            ast_node_free(left);
            return NULL;
        }
        
        printf("DEBUG: parse_comma_expression - parsed right side, creating comma node\n");
        
        /* Create comma expression node (for HolyC string formatting) */
        ASTNode *comma_node = ast_node_new(NODE_BINARY_OP, parser_current_line(parser), parser_current_column(parser));
        if (!comma_node) {
            ast_node_free(left);
            ast_node_free(right);
            return NULL;
        }
        
        comma_node->data.binary_op.left = left;
        comma_node->data.binary_op.right = right;
        comma_node->data.binary_op.op = BINOP_COMMA;
        
        printf("DEBUG: parse_comma_expression - comma node created successfully\n");
        return comma_node;
    }
    
    printf("DEBUG: parse_comma_expression - no comma, returning left side\n");
    return left;
}

ASTNode* parse_assignment_expression(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: parse_assignment_expression - starting, current token: %d\n", parser_current_token(parser));
    
    /* Parse conditional expressions first */
    ASTNode *left = parse_conditional_expression(parser);
    if (!left) {
        printf("DEBUG: parse_assignment_expression - failed to parse conditional expression\n");
        return NULL;
    }
    
    printf("DEBUG: parse_assignment_expression - parsed conditional expression, current token: %d\n", parser_current_token(parser));
    
    /* Parse assignment operators */
    while (parser_current_token(parser) == '=' || 
           parser_current_token(parser) == TK_ADD_EQU ||
           parser_current_token(parser) == TK_SUB_EQU ||
           parser_current_token(parser) == TK_MUL_EQU ||
           parser_current_token(parser) == TK_DIV_EQU ||
           parser_current_token(parser) == TK_MOD_EQU ||
           parser_current_token(parser) == TK_AND_EQU ||
           parser_current_token(parser) == TK_OR_EQU ||
           parser_current_token(parser) == TK_XOR_EQU ||
           parser_current_token(parser) == TK_SHL_EQU ||
           parser_current_token(parser) == TK_SHR_EQU) {
        
        SchismTokenType op = parser_current_token(parser);
        parser_next_token(parser); /* Consume operator */
        
        ASTNode *right = parse_assignment_expression(parser);
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
        
        /* Map token to binary operator */
        switch (op) {
            case '=': binop->data.binary_op.op = BINOP_ASSIGN; break;
            case TK_ADD_EQU: binop->data.binary_op.op = BINOP_ADD_ASSIGN; break;
            case TK_SUB_EQU: binop->data.binary_op.op = BINOP_SUB_ASSIGN; break;
            case TK_MUL_EQU: binop->data.binary_op.op = BINOP_MUL_ASSIGN; break;
            case TK_DIV_EQU: binop->data.binary_op.op = BINOP_DIV_ASSIGN; break;
            case TK_MOD_EQU: binop->data.binary_op.op = BINOP_MOD_ASSIGN; break;
            case TK_AND_EQU: binop->data.binary_op.op = BINOP_AND_ASSIGN; break;
            case TK_OR_EQU: binop->data.binary_op.op = BINOP_OR_ASSIGN; break;
            case TK_XOR_EQU: binop->data.binary_op.op = BINOP_XOR_ASSIGN; break;
            case TK_SHL_EQU: binop->data.binary_op.op = BINOP_SHL_ASSIGN; break;
            case TK_SHR_EQU: binop->data.binary_op.op = BINOP_SHR_ASSIGN; break;
            default: binop->data.binary_op.op = BINOP_ASSIGN; break;
        }
        
        binop->data.binary_op.left = left;
        binop->data.binary_op.right = right;
        
        left = binop;
    }
    
    return left;
}

ASTNode* parse_conditional_expression(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: parse_conditional_expression - starting, current token: %d\n", parser_current_token(parser));
    
    /* Parse logical OR expressions first */
    ASTNode *condition = parse_logical_or_expression(parser);
    if (!condition) {
        printf("DEBUG: parse_conditional_expression - failed to parse logical OR expression\n");
        return NULL;
    }
    
    printf("DEBUG: parse_conditional_expression - parsed logical OR expression, current token: %d\n", parser_current_token(parser));
    
    /* Check for ternary operator (condition ? true_expr : false_expr) */
    if (parser_current_token(parser) == '?') {
        parser_next_token(parser); /* Consume '?' */
        
        ASTNode *true_expr = parse_expression(parser);
        if (!true_expr) {
            ast_node_free(condition);
            return NULL;
        }
        
        if (parser_current_token(parser) != ':') {
            parser_error(parser, (U8*)"Expected ':' in ternary operator");
            ast_node_free(condition);
            ast_node_free(true_expr);
            return NULL;
        }
        parser_next_token(parser); /* Consume ':' */
        
        ASTNode *false_expr = parse_conditional_expression(parser);
        if (!false_expr) {
            ast_node_free(condition);
            ast_node_free(true_expr);
            return NULL;
        }
        
        /* Create conditional expression node */
        ASTNode *cond_node = ast_node_new(NODE_CONDITIONAL, parser_current_line(parser), parser_current_column(parser));
        if (!cond_node) {
            ast_node_free(condition);
            ast_node_free(true_expr);
            ast_node_free(false_expr);
            return NULL;
        }
        
        cond_node->data.conditional.condition = condition;
        cond_node->data.conditional.true_expr = true_expr;
        cond_node->data.conditional.false_expr = false_expr;
        
        return cond_node;
    }
    
    return condition;
}
ASTNode* parse_logical_or_expression(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: parse_logical_or_expression - starting, current token: %d\n", parser_current_token(parser));
    
    /* Parse logical XOR expressions first */
    ASTNode *left = parse_logical_xor_expression(parser);
    if (!left) {
        printf("DEBUG: parse_logical_or_expression - failed to parse logical XOR expression\n");
        return NULL;
    }
    
    printf("DEBUG: parse_logical_or_expression - parsed logical XOR expression, current token: %d\n", parser_current_token(parser));
    
    /* Parse logical OR operators (||) */
    while (parser_current_token(parser) == TK_OR_OR) {
        parser_next_token(parser); /* Consume || */
        
        ASTNode *right = parse_logical_xor_expression(parser);
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
    
    /* Parse bitwise OR expressions first */
    ASTNode *left = parse_bitwise_or_expression(parser);
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

ASTNode* parse_logical_xor_expression(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: parse_logical_xor_expression - starting, current token: %d\n", parser_current_token(parser));
    
    /* Parse logical AND expressions first */
    ASTNode *left = parse_logical_and_expression(parser);
    if (!left) {
        printf("DEBUG: parse_logical_xor_expression - failed to parse logical AND expression\n");
        return NULL;
    }
    
    printf("DEBUG: parse_logical_xor_expression - parsed logical AND expression, current token: %d\n", parser_current_token(parser));
    
    /* Parse logical XOR operators (^^) */
    while (parser_current_token(parser) == TK_XOR_XOR) {
        parser_next_token(parser); /* Consume ^^ */
        
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
        
        binop->data.binary_op.op = BINOP_XOR_XOR;
        binop->data.binary_op.left = left;
        binop->data.binary_op.right = right;
        
        left = binop;
    }
    
    return left;
}

ASTNode* parse_bitwise_or_expression(ParserState *parser) {
    if (!parser) return NULL;
    
    /* Parse bitwise XOR expressions first */
    ASTNode *left = parse_bitwise_xor_expression(parser);
    if (!left) return NULL;
    
    /* Parse bitwise OR operators (|) */
    while (parser_current_token(parser) == '|') {
        parser_next_token(parser); /* Consume | */
        
        ASTNode *right = parse_bitwise_xor_expression(parser);
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
        
        binop->data.binary_op.op = BINOP_OR;
        binop->data.binary_op.left = left;
        binop->data.binary_op.right = right;
        
        left = binop;
    }
    
    return left;
}

ASTNode* parse_bitwise_xor_expression(ParserState *parser) {
    if (!parser) return NULL;
    
    /* Parse bitwise AND expressions first */
    ASTNode *left = parse_bitwise_and_expression(parser);
    if (!left) return NULL;
    
    /* Parse bitwise XOR operators (^) */
    while (parser_current_token(parser) == '^') {
        parser_next_token(parser); /* Consume ^ */
        
        ASTNode *right = parse_bitwise_and_expression(parser);
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
        
        binop->data.binary_op.op = BINOP_XOR;
        binop->data.binary_op.left = left;
        binop->data.binary_op.right = right;
        
        left = binop;
    }
    
    return left;
}

ASTNode* parse_bitwise_and_expression(ParserState *parser) {
    if (!parser) return NULL;
    
    /* Parse equality expressions first */
    ASTNode *left = parse_equality_expression(parser);
    if (!left) return NULL;
    
    /* Parse bitwise AND operators (&) */
    while (parser_current_token(parser) == '&') {
        parser_next_token(parser); /* Consume & */
        
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
        
        binop->data.binary_op.op = BINOP_AND;
        binop->data.binary_op.left = left;
        binop->data.binary_op.right = right;
        
        left = binop;
    }
    
    return left;
}
ASTNode* parse_equality_expression(ParserState *parser) {
    if (!parser) return NULL;
    
    /* Parse relational expressions first */
    ASTNode *left = parse_relational_expression(parser);
    if (!left) return NULL;
    
    /* Parse equality operators (==, !=) */
    while (parser_current_token(parser) == TK_EQU_EQU || parser_current_token(parser) == TK_NOT_EQU) {
        SchismTokenType op = parser_current_token(parser);
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
    
    printf("DEBUG: parse_relational_expression - starting, current token: %d\n", parser_current_token(parser));
    
    /* Parse shift expressions first */
    ASTNode *left = parse_shift_expression(parser);
    if (!left) return NULL;
    
    /* Check if this is a range comparison (multiple consecutive comparison operators) */
    if (parser_current_token(parser) == '<' || parser_current_token(parser) == '>' ||
        parser_current_token(parser) == TK_LESS_EQU || parser_current_token(parser) == TK_GREATER_EQU) {
        
        /* Check if the next token after the comparison is also a comparison operator */
        /* This indicates a range comparison like 5<i<j+1<20 */
        parser_save_position(parser);
        SchismTokenType first_op = parser_current_token(parser);
        parser_next_token(parser); /* consume first comparison operator */
        
        /* Parse the next expression */
        ASTNode *second_expr = parse_shift_expression(parser);
        if (second_expr) {
            /* Check if there's another comparison operator */
            if (parser_current_token(parser) == '<' || parser_current_token(parser) == '>' ||
                parser_current_token(parser) == TK_LESS_EQU || parser_current_token(parser) == TK_GREATER_EQU) {
                
                /* This is a range comparison! Parse the entire range */
                parser_restore_position(parser);
                return parse_range_comparison(parser, left);
            }
        }
        
        /* Not a range comparison, restore position and continue with normal parsing */
        parser_restore_position(parser);
    }
    
    /* Parse relational operators (<, >, <=, >=) - normal single comparison */
    while (parser_current_token(parser) == '<' || parser_current_token(parser) == '>' ||
           parser_current_token(parser) == TK_LESS_EQU || parser_current_token(parser) == TK_GREATER_EQU) {
        SchismTokenType op = parser_current_token(parser);
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
        SchismTokenType op = parser_current_token(parser);
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
    
    SchismTokenType current = parser_current_token(parser);
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
        
        /* Check for variable arguments (...) */
        if (parser_current_token(parser) == TK_ELLIPSIS) {
            printf("DEBUG: Found variable arguments (...)\n");
            parser_next_token(parser); /* consume '...' */
            
            /* Create variable arguments node */
            ASTNode *varargs_node = ast_node_new(NODE_VARARGS, parser_current_line(parser), parser_current_column(parser));
            if (varargs_node) {
                varargs_node->data.varargs.type = (U8*)"...";
                varargs_node->data.varargs.arg_index = param_count;
                varargs_node->data.varargs.is_va_list = true;
                
                /* Add to parameter list */
                ast_node_add_child(param_list, varargs_node);
            }
            break; /* Variable arguments must be the last parameter */
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
        
        /* Check for default argument value */
        ASTNode *default_value = NULL;
        if (parser_current_token(parser) == '=') {
            printf("DEBUG: Found default argument value\n");
            parser_next_token(parser); /* consume '=' */
            default_value = parse_expression(parser);
            if (!default_value) {
                printf("DEBUG: Failed to parse default argument value\n");
            }
        }
        
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
        
        /* Create default argument node if we have a default value */
        if (default_value) {
            ASTNode *default_arg_node = ast_node_new(NODE_DEFAULT_ARG, parser_current_line(parser), parser_current_column(parser));
            if (default_arg_node) {
                default_arg_node->data.default_arg.parameter = param_var;
                default_arg_node->data.default_arg.default_value = default_value;
                ast_node_add_child(param_list, default_arg_node);
            }
        } else {
            /* Add regular parameter to list */
            ast_node_add_child(param_list, param_var);
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

/* Parse variable arguments (...) */
ASTNode* parse_variable_arguments(ParserState *parser) {
    if (!parser) return NULL;
    
    if (parser_current_token(parser) != TK_ELLIPSIS) {
        return NULL;
    }
    
    printf("DEBUG: Parsing variable arguments (...)\n");
    parser_next_token(parser); /* consume '...' */
    
    ASTNode *varargs_node = ast_node_new(NODE_VARARGS, parser_current_line(parser), parser_current_column(parser));
    if (!varargs_node) return NULL;
    
    varargs_node->data.varargs.type = (U8*)"...";
    varargs_node->data.varargs.arg_index = 0; /* Will be set by caller */
    varargs_node->data.varargs.is_va_list = true;
    
    return varargs_node;
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
    
    SchismTokenType current = parser_current_token(parser);
    
    /* Check for type tokens */
    if (current >= TK_TYPE_I0 && current <= TK_TYPE_STRING) {
        ASTNode *type_node = ast_node_new(NODE_TYPE_SPECIFIER, parser_current_line(parser), parser_current_column(parser));
        if (!type_node) return NULL;
        
        /* Set the type information */
        type_node->data.type_specifier.type = current;
        
        /* Move to next token */
        parser_next_token(parser);
        
        /* Check for pointer type (*) */
        while (parser_current_token(parser) == '*') {
            parser_next_token(parser); /* consume '*' */
            /* Mark as pointer type - we'll handle this in the type system */
            type_node->data.type_specifier.is_pointer = true;
        }
        
        return type_node;
    }
    
    /* No type found */
    return NULL;
}
ASTNode* parse_assembly_block(ParserState *parser) { return NULL; }

/* Parse inline assembly block: asm { ... } */
ASTNode* parse_inline_assembly_block(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: Parsing inline assembly block\n");
    
    /* Expect opening brace */
    if (parser_current_token(parser) != '{') {
        parser_error(parser, (U8*)"Expected '{' after asm");
        return NULL;
    }
    parser_next_token(parser); /* consume '{' */
    
    ASTNode *asm_node = ast_node_new(NODE_INLINE_ASM, parser_current_line(parser), parser_current_column(parser));
    if (!asm_node) return NULL;
    
    /* Initialize inline assembly data */
    asm_node->data.inline_asm.instructions = NULL;
    asm_node->data.inline_asm.is_volatile = false;
    asm_node->data.inline_asm.input_count = 0;
    asm_node->data.inline_asm.output_count = 0;
    asm_node->data.inline_asm.clobber_count = 0;
    asm_node->data.inline_asm.input_ops = NULL;
    asm_node->data.inline_asm.output_ops = NULL;
    asm_node->data.inline_asm.clobber_ops = NULL;
    
    /* Parse assembly instructions until closing brace */
    while (parser_current_token(parser) != '}' && parser_current_token(parser) != TK_EOF) {
        /* For now, just skip tokens until we find the closing brace */
        /* TODO: Implement proper assembly instruction parsing */
        parser_next_token(parser);
    }
    
    if (parser_current_token(parser) == '}') {
        parser_next_token(parser); /* consume '}' */
    }
    
    printf("DEBUG: Completed inline assembly block parsing\n");
    return asm_node;
}

/* Parse register directive: reg/noreg */
ASTNode* parse_register_directive(ParserState *parser) {
    if (!parser) return NULL;
    
    SchismTokenType current = parser_current_token(parser);
    printf("DEBUG: Parsing register directive: %s\n", (current == TK_REG) ? "reg" : "noreg");
    
    ASTNode *reg_node = ast_node_new(NODE_REG_DIRECTIVE, parser_current_line(parser), parser_current_column(parser));
    if (!reg_node) return NULL;
    
    reg_node->data.reg_directive.is_reg = (current == TK_REG);
    reg_node->data.reg_directive.function_name = NULL; /* Will be set by caller if needed */
    
    parser_next_token(parser); /* consume reg/noreg */
    
    return reg_node;
}

/* Parse try block: try { ... } catch (type name) { ... } */
ASTNode* parse_try_block(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: Parsing try block\n");
    
    if (parser_current_token(parser) != TK_TRY) {
        parser_error(parser, (U8*)"Expected 'try'");
        return NULL;
    }
    parser_next_token(parser); /* consume 'try' */
    
    ASTNode *try_node = ast_node_new(NODE_TRY_BLOCK, parser_current_line(parser), parser_current_column(parser));
    if (!try_node) return NULL;
    
    /* Parse try body */
    if (parser_current_token(parser) != '{') {
        parser_error(parser, (U8*)"Expected '{' after try");
        ast_node_free(try_node);
        return NULL;
    }
    try_node->data.try_block.try_body = parse_block_statement(parser);
    
    /* Initialize catch blocks */
    try_node->data.try_block.catch_blocks = NULL;
    try_node->data.try_block.catch_count = 0;
    
    /* Parse catch blocks */
    while (parser_current_token(parser) == TK_CATCH) {
        ASTNode *catch_block = parse_catch_block(parser);
        if (catch_block) {
            ast_node_add_child(try_node, catch_block);
            try_node->data.try_block.catch_count++;
        }
    }
    
    return try_node;
}

/* Parse catch block: catch (type name) { ... } */
ASTNode* parse_catch_block(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: Parsing catch block\n");
    
    if (parser_current_token(parser) != TK_CATCH) {
        parser_error(parser, (U8*)"Expected 'catch'");
        return NULL;
    }
    parser_next_token(parser); /* consume 'catch' */
    
    ASTNode *catch_node = ast_node_new(NODE_CATCH_BLOCK, parser_current_line(parser), parser_current_column(parser));
    if (!catch_node) return NULL;
    
    /* Parse exception type and name */
    if (parser_current_token(parser) != '(') {
        parser_error(parser, (U8*)"Expected '(' after catch");
        ast_node_free(catch_node);
        return NULL;
    }
    parser_next_token(parser); /* consume '(' */
    
    /* Parse exception type */
    ASTNode *exception_type_node = parse_type_specifier(parser);
    if (!exception_type_node) {
        parser_error(parser, (U8*)"Expected exception type");
        ast_node_free(catch_node);
        return NULL;
    }
    catch_node->data.catch_block.exception_type = (U8*)exception_type_node->data.type_specifier.type_name;
    
    /* Parse exception name */
    if (parser_current_token(parser) != TK_IDENT) {
        parser_error(parser, (U8*)"Expected exception variable name");
        ast_node_free(catch_node);
        ast_node_free(exception_type_node);
        return NULL;
    }
    catch_node->data.catch_block.exception_name = parser_current_token_value(parser);
    parser_next_token(parser);
    
    if (parser_current_token(parser) != ')') {
        parser_error(parser, (U8*)"Expected ')' after exception name");
        ast_node_free(catch_node);
        ast_node_free(exception_type_node);
        return NULL;
    }
    parser_next_token(parser); /* consume ')' */
    
    /* Parse catch body */
    if (parser_current_token(parser) != '{') {
        parser_error(parser, (U8*)"Expected '{' after catch");
        ast_node_free(catch_node);
        ast_node_free(exception_type_node);
        return NULL;
    }
    catch_node->data.catch_block.catch_body = parse_block_statement(parser);
    
    ast_node_free(exception_type_node);
    return catch_node;
}

/* Parse throw statement: throw expression; */
ASTNode* parse_throw_statement(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: Parsing throw statement\n");
    
    if (parser_current_token(parser) != TK_THROW) {
        parser_error(parser, (U8*)"Expected 'throw'");
        return NULL;
    }
    parser_next_token(parser); /* consume 'throw' */
    
    ASTNode *throw_node = ast_node_new(NODE_THROW_STMT, parser_current_line(parser), parser_current_column(parser));
    if (!throw_node) return NULL;
    
    /* Parse exception expression */
    throw_node->data.throw_stmt.exception = parse_expression(parser);
    if (!throw_node->data.throw_stmt.exception) {
        parser_error(parser, (U8*)"Expected exception expression");
        ast_node_free(throw_node);
        return NULL;
    }
    
    throw_node->data.throw_stmt.exception_type = (U8*)"Exception"; /* Default exception type */
    
    return throw_node;
}

/* Parse type inference: auto variable = expression; */
ASTNode* parse_type_inference(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: Parsing type inference (auto)\n");
    
    if (parser_current_token(parser) != TK_AUTO) {
        parser_error(parser, (U8*)"Expected 'auto'");
        return NULL;
    }
    parser_next_token(parser); /* consume 'auto' */
    
    /* Parse variable name */
    if (parser_current_token(parser) != TK_IDENT) {
        parser_error(parser, (U8*)"Expected variable name after 'auto'");
        return NULL;
    }
    
    U8 *var_name = parser_current_token_value(parser);
    parser_next_token(parser); /* consume variable name */
    
    /* Expect assignment operator */
    if (parser_current_token(parser) != '=') {
        parser_error(parser, (U8*)"Expected '=' after auto variable name");
        return NULL;
    }
    parser_next_token(parser); /* consume '=' */
    
    /* Parse initializer expression */
    ASTNode *initializer = parse_expression(parser);
    if (!initializer) {
        parser_error(parser, (U8*)"Expected initializer expression");
        return NULL;
    }
    
    /* Create type inference node */
    ASTNode *inference_node = ast_node_new(NODE_TYPE_INFERENCE, parser_current_line(parser), parser_current_column(parser));
    if (!inference_node) {
        ast_node_free(initializer);
        return NULL;
    }
    
    inference_node->data.type_inference.expression = initializer;
    inference_node->data.type_inference.inferred_type = (U8*)"auto"; /* Will be determined later */
    inference_node->data.type_inference.is_auto = true;
    
    /* Create variable declaration node */
    ASTNode *var_node = ast_node_new(NODE_VARIABLE, parser_current_line(parser), parser_current_column(parser));
    if (var_node) {
        var_node->data.variable.name = var_name;
        var_node->data.variable.type = (U8*)"auto";
        var_node->data.variable.initializer = initializer;
        var_node->data.variable.is_parameter = false;
        var_node->data.variable.is_global = false;
        
        /* Add variable to scope */
        if (!scope_add_variable(parser_get_current_scope(parser), var_node)) {
            printf("WARNING: Failed to add auto variable to scope\n");
        }
    }
    
    return var_node;
}

/* Parse enhanced type casting: (type)expression */
ASTNode* parse_enhanced_type_cast(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: Parsing enhanced type cast\n");
    
    if (parser_current_token(parser) != '(') {
        return NULL;
    }
    parser_next_token(parser); /* consume '(' */
    
    /* Parse target type */
    ASTNode *target_type_node = parse_type_specifier(parser);
    if (!target_type_node) {
        parser_error(parser, (U8*)"Expected type in cast");
        return NULL;
    }
    
    if (parser_current_token(parser) != ')') {
        parser_error(parser, (U8*)"Expected ')' after type in cast");
        ast_node_free(target_type_node);
        return NULL;
    }
    parser_next_token(parser); /* consume ')' */
    
    /* Parse expression to cast */
    ASTNode *expression = parse_expression(parser);
    if (!expression) {
        parser_error(parser, (U8*)"Expected expression after cast");
        ast_node_free(target_type_node);
        return NULL;
    }
    
    /* Create enhanced cast node */
    ASTNode *cast_node = ast_node_new(NODE_ENHANCED_CAST, parser_current_line(parser), parser_current_column(parser));
    if (!cast_node) {
        ast_node_free(target_type_node);
        ast_node_free(expression);
        return NULL;
    }
    
    cast_node->data.enhanced_cast.target_type = (U8*)target_type_node->data.type_specifier.type_name;
    cast_node->data.enhanced_cast.expression = expression;
    cast_node->data.enhanced_cast.is_explicit = true;
    cast_node->data.enhanced_cast.is_const_cast = false;
    cast_node->data.enhanced_cast.is_reinterpret_cast = false;
    
    ast_node_free(target_type_node);
    return cast_node;
}

/* Check if a function is defined in the current scope */
Bool parser_is_function_defined_in_scope(ParserState *parser, U8 *name) {
    if (!parser || !name) return false;
    
    /* For now, just return false - this would need to be implemented */
    /* with proper symbol table lookup */
    return false;
}

ASTNode* parse_assembly_instruction(ParserState *parser) { return NULL; }
ASTNode* parse_assembly_operand(ParserState *parser) { return NULL; }
ASTNode* parse_assembly_register(ParserState *parser) { return NULL; }
ASTNode* parse_assembly_memory(ParserState *parser) { return NULL; }
ASTNode* parse_assembly_immediate(ParserState *parser) { return NULL; }
ASTNode* parse_assembly_label(ParserState *parser) { return NULL; }
ASTNode* parse_range_expression(ParserState *parser) { return NULL; }
ASTNode* parse_dollar_expression(ParserState *parser) { return NULL; }
ASTNode* parse_class_definition(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: Parsing class definition, current token: %d\n", parser_current_token(parser));
    
    I64 line = parser_current_line(parser);
    I64 column = parser_current_column(parser);
    
    Bool is_public = false;
    Bool is_union = false;
    
    /* Check for 'public' keyword */
    if (parser_current_token(parser) == TK_PUBLIC) {
        is_public = true;
        parser_next_token(parser); /* consume 'public' */
    }
    
    /* Check for 'class' or 'union' keyword */
    if (parser_current_token(parser) == TK_CLASS) {
        is_union = false;
        parser_next_token(parser); /* consume 'class' */
    } else if (parser_current_token(parser) == TK_UNION) {
        is_union = true;
        parser_next_token(parser); /* consume 'union' */
    } else {
        parser_error(parser, (U8*)"Expected 'class' or 'union' keyword");
        return NULL;
    }
    
    /* Parse class name */
    if (parser_current_token(parser) != TK_IDENT) {
        parser_error(parser, (U8*)"Expected class name");
        return NULL;
    }
    
    U8 *class_name = parser_current_token_value(parser);
    if (!class_name) {
        parser_error(parser, (U8*)"Failed to get class name");
        return NULL;
    }
    parser_next_token(parser); /* consume class name */
    
    /* Check for inheritance */
    U8 *base_class = NULL;
    if (parser_current_token(parser) == ':') {
        parser_next_token(parser); /* consume ':' */
        
        if (parser_current_token(parser) != TK_IDENT) {
            parser_error(parser, (U8*)"Expected base class name");
            return NULL;
        }
        
        base_class = parser_current_token_value(parser);
        if (!base_class) {
            parser_error(parser, (U8*)"Failed to get base class name");
            return NULL;
        }
        parser_next_token(parser); /* consume base class name */
    }
    
    /* Expect opening brace */
    if (parser_current_token(parser) != '{') {
        parser_error(parser, (U8*)"Expected '{' after class name");
        return NULL;
    }
    parser_next_token(parser); /* consume '{' */
    
    /* Parse member declarations */
    ASTNode *members = NULL;
    I64 member_count = 0;
    
    while (parser_current_token(parser) != '}' && parser_current_token(parser) != 0) {
        /* Parse member declaration (type + identifier) */
        ASTNode *member_type = parse_type_specifier(parser);
        if (!member_type) {
            parser_error(parser, (U8*)"Expected member type");
            break;
        }
        
        /* Parse member name */
        if (parser_current_token(parser) != TK_IDENT) {
            parser_error(parser, (U8*)"Expected member name");
            ast_node_free(member_type);
            break;
        }
        
        U8 *member_name = parser_current_token_value(parser);
        if (!member_name) {
            parser_error(parser, (U8*)"Failed to get member name");
            ast_node_free(member_type);
            break;
        }
        parser_next_token(parser); /* consume member name */
        
        /* Expect semicolon */
        if (parser_current_token(parser) != ';') {
            parser_error(parser, (U8*)"Expected ';' after member declaration");
            ast_node_free(member_type);
            break;
        }
        parser_next_token(parser); /* consume ';' */
        
        /* Create member node */
        ASTNode *member_node = ast_node_new(NODE_VARIABLE, line, column);
        if (member_node) {
            member_node->data.variable.type = member_type;
            member_node->data.variable.name = member_name;
            
            /* Add to members list */
            if (!members) {
                members = member_node;
            } else {
                /* Add to end of list */
                ASTNode *current = members;
                while (current->next) {
                    current = current->next;
                }
                current->next = member_node;
            }
            member_count++;
        }
    }
    
    /* Expect closing brace */
    if (parser_current_token(parser) != '}') {
        parser_error(parser, (U8*)"Expected '}' after class members");
        return NULL;
    }
    parser_next_token(parser); /* consume '}' */
    
    /* Expect semicolon */
    if (parser_current_token(parser) != ';') {
        parser_error(parser, (U8*)"Expected ';' after class definition");
        return NULL;
    }
    parser_next_token(parser); /* consume ';' */
    
    /* Create class definition node */
    ASTNode *class_node = ast_node_new(NODE_CLASS_DEF, line, column);
    if (!class_node) {
        return NULL;
    }
    
    /* Initialize class definition data */
    class_node->data.class_def.class_name = class_name;
    class_node->data.class_def.base_class = base_class;
    class_node->data.class_def.members = members;
    class_node->data.class_def.member_count = member_count;
    class_node->data.class_def.is_public = is_public;
    class_node->data.class_def.is_union = is_union;
    
    printf("DEBUG: Class definition parsed successfully, name: %s, members: %ld, public: %s, union: %s\n", 
           class_name, member_count, is_public ? "true" : "false", is_union ? "true" : "false");
    return class_node;
}
ASTNode* parse_union_definition(ParserState *parser) { return NULL; }
ASTNode* parse_public_declaration(ParserState *parser) { return NULL; }
ASTNode* parse_extern_declaration(ParserState *parser) { return NULL; }
ASTNode* parse_import_declaration(ParserState *parser) { return NULL; }
ASTNode* parse_try_catch_block(ParserState *parser) { return NULL; }
ASTNode* parse_type_declaration(ParserState *parser) { return NULL; }
ASTNode* parse_type_cast(ParserState *parser) { return NULL; }
ASTNode* parse_type_reference(ParserState *parser) { return NULL; }
ASTNode* parse_type_dereference(ParserState *parser) { return NULL; }
ASTNode* parse_array_access(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: Parsing array access, current token: %d\n", parser_current_token(parser));
    
    I64 line = parser_current_line(parser);
    I64 column = parser_current_column(parser);
    
    /* Expect '[' */
    if (parser_current_token(parser) != '[') {
        parser_error(parser, (U8*)"Expected '[' for array access");
        return NULL;
    }
    parser_next_token(parser); /* consume '[' */
    
    /* Parse index expression */
    ASTNode *index = parse_expression(parser);
    if (!index) {
        parser_error(parser, (U8*)"Expected index expression in array access");
        return NULL;
    }
    
    /* Expect ']' */
    if (parser_current_token(parser) != ']') {
        parser_error(parser, (U8*)"Expected ']' after array index");
        ast_node_free(index);
        return NULL;
    }
    parser_next_token(parser); /* consume ']' */
    
    /* Create array access node */
    ASTNode *array_access = ast_node_new(NODE_ARRAY_ACCESS, line, column);
    if (!array_access) {
        ast_node_free(index);
        return NULL;
    }
    
    /* Initialize array access data */
    array_access->data.array_access.array = NULL; /* Will be set by caller */
    array_access->data.array_access.index = index;
    
    printf("DEBUG: Array access parsed successfully\n");
    return array_access;
}
ASTNode* parse_member_access(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: Parsing member access, current token: %d\n", parser_current_token(parser));
    
    I64 line = parser_current_line(parser);
    I64 column = parser_current_column(parser);
    
    /* Expect '.' or '->' */
    if (parser_current_token(parser) != '.' && parser_current_token(parser) != TK_DEREFERENCE) {
        parser_error(parser, (U8*)"Expected '.' or '->' for member access");
        return NULL;
    }
    
    Bool is_arrow = (parser_current_token(parser) == TK_DEREFERENCE);
    parser_next_token(parser); /* consume '.' or '->' */
    
    /* Parse member name */
    if (parser_current_token(parser) != TK_IDENT) {
        parser_error(parser, (U8*)"Expected member name after '.' or '->'");
        return NULL;
    }
    
    U8 *member_name = parser_current_token_value(parser);
    if (!member_name) {
        parser_error(parser, (U8*)"Failed to get member name");
        return NULL;
    }
    parser_next_token(parser); /* consume member name */
    
    /* Create member access node */
    ASTNode *member_node = ast_node_new(NODE_MEMBER_ACCESS, line, column);
    if (!member_node) {
        return NULL;
    }
    
    /* Initialize member access data */
    member_node->data.member_access.object = NULL; /* Will be set by caller */
    member_node->data.member_access.member_name = member_name;
    
    printf("DEBUG: Member access parsed successfully, member: %s, arrow: %s\n", 
           member_name, is_arrow ? "true" : "false");
    return member_node;
}
ASTNode* parse_pointer_arithmetic(ParserState *parser) { return NULL; }

/* Sub-int access parsing functions */

/* Check if current token pattern matches sub-int access (identifier.type[expr]) */
Bool is_sub_int_access_pattern(ParserState *parser) {
    if (!parser) return false;
    
    /* Save current position */
    parser_save_position(parser);
    
    /* Check pattern: identifier . type [ expression ] */
    if (parser_current_token(parser) != TK_IDENT) {
        parser_restore_position(parser);
        return false;
    }
    parser_next_token(parser); /* consume identifier */
    
    if (parser_current_token(parser) != '.') {
        parser_restore_position(parser);
        return false;
    }
    parser_next_token(parser); /* consume '.' */
    
    /* Check if next token is a type identifier */
    if (parser_current_token(parser) != TK_IDENT) {
        parser_restore_position(parser);
        return false;
    }
    
    U8 *type_name = parser_current_token_value(parser);
    if (!type_name) {
        parser_restore_position(parser);
        return false;
    }
    
    /* Check if it's a valid sub-int type (i8, u8, i16, u16, i32, u32) */
    if (strcmp(type_name, "i8") != 0 && strcmp(type_name, "u8") != 0 &&
        strcmp(type_name, "i16") != 0 && strcmp(type_name, "u16") != 0 &&
        strcmp(type_name, "i32") != 0 && strcmp(type_name, "u32") != 0) {
        parser_restore_position(parser);
        return false;
    }
    parser_next_token(parser); /* consume type */
    
    if (parser_current_token(parser) != '[') {
        parser_restore_position(parser);
        return false;
    }
    
    /* Restore position and return true */
    parser_restore_position(parser);
    return true;
}

/* Parse sub-int access expression (i.u16[1]) */
ASTNode* parse_sub_int_access(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: Parsing sub-int access, current token: %d\n", parser_current_token(parser));
    
    I64 line = parser_current_line(parser);
    I64 column = parser_current_column(parser);
    
    /* Parse base object (identifier) */
    if (parser_current_token(parser) != TK_IDENT) {
        parser_error(parser, (U8*)"Expected identifier for sub-int access");
        return NULL;
    }
    
    U8 *object_name = parser_current_token_value(parser);
    if (!object_name) {
        parser_error(parser, (U8*)"Failed to get object name");
        return NULL;
    }
    parser_next_token(parser); /* consume identifier */
    
    /* Create base object node */
    ASTNode *base_object = ast_node_new(NODE_IDENTIFIER, line, column);
    if (!base_object) {
        parser_error(parser, (U8*)"Failed to create base object node");
        return NULL;
    }
    base_object->data.identifier.name = strdup(object_name);
    
    /* Expect '.' */
    if (parser_current_token(parser) != '.') {
        parser_error(parser, (U8*)"Expected '.' after object name");
        ast_node_free(base_object);
        return NULL;
    }
    parser_next_token(parser); /* consume '.' */
    
    /* Parse member type */
    if (parser_current_token(parser) != TK_IDENT) {
        parser_error(parser, (U8*)"Expected member type after '.'");
        ast_node_free(base_object);
        return NULL;
    }
    
    U8 *member_type = parser_current_token_value(parser);
    if (!member_type) {
        parser_error(parser, (U8*)"Failed to get member type");
        ast_node_free(base_object);
        return NULL;
    }
    parser_next_token(parser); /* consume member type */
    
    /* Expect '[' */
    if (parser_current_token(parser) != '[') {
        parser_error(parser, (U8*)"Expected '[' after member type");
        ast_node_free(base_object);
        return NULL;
    }
    parser_next_token(parser); /* consume '[' */
    
    /* Parse index expression */
    ASTNode *index = parse_expression(parser);
    if (!index) {
        parser_error(parser, (U8*)"Expected index expression in sub-int access");
        ast_node_free(base_object);
        return NULL;
    }
    
    /* Expect ']' */
    if (parser_current_token(parser) != ']') {
        parser_error(parser, (U8*)"Expected ']' after index expression");
        ast_node_free(base_object);
        ast_node_free(index);
        return NULL;
    }
    parser_next_token(parser); /* consume ']' */
    
    /* Create sub-int access node */
    ASTNode *sub_int_node = ast_node_new(NODE_SUB_INT_ACCESS, line, column);
    if (!sub_int_node) {
        ast_node_free(base_object);
        ast_node_free(index);
        return NULL;
    }
    
    /* Initialize sub-int access data */
    sub_int_node->data.sub_int_access.base_object = base_object;
    sub_int_node->data.sub_int_access.member_type = strdup(member_type);
    sub_int_node->data.sub_int_access.index = index;
    
    /* Calculate member size and properties */
    if (strcmp(member_type, "i8") == 0 || strcmp(member_type, "u8") == 0) {
        sub_int_node->data.sub_int_access.member_size = 1;
        sub_int_node->data.sub_int_access.is_signed = (strcmp(member_type, "i8") == 0);
    } else if (strcmp(member_type, "i16") == 0 || strcmp(member_type, "u16") == 0) {
        sub_int_node->data.sub_int_access.member_size = 2;
        sub_int_node->data.sub_int_access.is_signed = (strcmp(member_type, "i16") == 0);
    } else if (strcmp(member_type, "i32") == 0 || strcmp(member_type, "u32") == 0) {
        sub_int_node->data.sub_int_access.member_size = 4;
        sub_int_node->data.sub_int_access.is_signed = (strcmp(member_type, "i32") == 0);
    } else {
        parser_error(parser, (U8*)"Invalid member type for sub-int access");
        ast_node_free(base_object);
        ast_node_free(index);
        ast_node_free(sub_int_node);
        return NULL;
    }
    
    /* Calculate member offset (will be calculated at runtime based on index) */
    sub_int_node->data.sub_int_access.member_offset = 0;
    
    printf("DEBUG: Sub-int access parsed successfully: %s.%s[expr], size: %lld, signed: %s\n", 
           object_name, member_type, sub_int_node->data.sub_int_access.member_size,
           sub_int_node->data.sub_int_access.is_signed ? "true" : "false");
    
    return sub_int_node;
}

/* Parse union member access with array notation */
ASTNode* parse_union_member_access(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: Parsing union member access, current token: %d\n", parser_current_token(parser));
    
    I64 line = parser_current_line(parser);
    I64 column = parser_current_column(parser);
    
    /* Parse union object */
    if (parser_current_token(parser) != TK_IDENT) {
        parser_error(parser, (U8*)"Expected union object identifier");
        return NULL;
    }
    
    U8 *union_name = parser_current_token_value(parser);
    if (!union_name) {
        parser_error(parser, (U8*)"Failed to get union object name");
        return NULL;
    }
    parser_next_token(parser); /* consume union name */
    
    /* Create union object node */
    ASTNode *union_object = ast_node_new(NODE_IDENTIFIER, line, column);
    if (!union_object) {
        parser_error(parser, (U8*)"Failed to create union object node");
        return NULL;
    }
    union_object->data.identifier.name = strdup(union_name);
    
    /* Expect '.' */
    if (parser_current_token(parser) != '.') {
        parser_error(parser, (U8*)"Expected '.' after union object");
        ast_node_free(union_object);
        return NULL;
    }
    parser_next_token(parser); /* consume '.' */
    
    /* Parse member name */
    if (parser_current_token(parser) != TK_IDENT) {
        parser_error(parser, (U8*)"Expected member name after '.'");
        ast_node_free(union_object);
        return NULL;
    }
    
    U8 *member_name = parser_current_token_value(parser);
    if (!member_name) {
        parser_error(parser, (U8*)"Failed to get member name");
        ast_node_free(union_object);
        return NULL;
    }
    parser_next_token(parser); /* consume member name */
    
    /* Expect '[' */
    if (parser_current_token(parser) != '[') {
        parser_error(parser, (U8*)"Expected '[' after member name");
        ast_node_free(union_object);
        return NULL;
    }
    parser_next_token(parser); /* consume '[' */
    
    /* Parse index expression */
    ASTNode *index = parse_expression(parser);
    if (!index) {
        parser_error(parser, (U8*)"Expected index expression in union member access");
        ast_node_free(union_object);
        return NULL;
    }
    
    /* Expect ']' */
    if (parser_current_token(parser) != ']') {
        parser_error(parser, (U8*)"Expected ']' after index expression");
        ast_node_free(union_object);
        ast_node_free(index);
        return NULL;
    }
    parser_next_token(parser); /* consume ']' */
    
    /* Create union member access node */
    ASTNode *union_member_node = ast_node_new(NODE_UNION_MEMBER_ACCESS, line, column);
    if (!union_member_node) {
        ast_node_free(union_object);
        ast_node_free(index);
        return NULL;
    }
    
    /* Initialize union member access data */
    union_member_node->data.union_member_access.union_object = union_object;
    union_member_node->data.union_member_access.member_name = strdup(member_name);
    union_member_node->data.union_member_access.index = index;
    union_member_node->data.union_member_access.member_size = 0; /* Will be determined by type checking */
    union_member_node->data.union_member_access.member_offset = 0; /* Will be calculated */
    
    printf("DEBUG: Union member access parsed successfully: %s.%s[expr]\n", 
           union_name, member_name);
    
    return union_member_node;
}

/* Parse type-prefixed union declaration (public I64i union I64) */
ASTNode* parse_type_prefixed_union(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: Parsing type-prefixed union, current token: %d\n", parser_current_token(parser));
    
    I64 line = parser_current_line(parser);
    I64 column = parser_current_column(parser);
    
    Bool is_public = false;
    
    /* Check for 'public' keyword */
    if (parser_current_token(parser) == TK_PUBLIC) {
        is_public = true;
        parser_next_token(parser); /* consume 'public' */
    }
    
    /* Parse prefix type (e.g., I64i) */
    if (parser_current_token(parser) != TK_IDENT) {
        parser_error(parser, (U8*)"Expected prefix type for type-prefixed union");
        return NULL;
    }
    
    U8 *prefix_type = parser_current_token_value(parser);
    if (!prefix_type) {
        parser_error(parser, (U8*)"Failed to get prefix type");
        return NULL;
    }
    parser_next_token(parser); /* consume prefix type */
    
    /* Expect 'union' keyword */
    if (parser_current_token(parser) != TK_UNION) {
        parser_error(parser, (U8*)"Expected 'union' keyword after prefix type");
        return NULL;
    }
    parser_next_token(parser); /* consume 'union' */
    
    /* Parse union name */
    if (parser_current_token(parser) != TK_IDENT) {
        parser_error(parser, (U8*)"Expected union name");
        return NULL;
    }
    
    U8 *union_name = parser_current_token_value(parser);
    if (!union_name) {
        parser_error(parser, (U8*)"Failed to get union name");
        return NULL;
    }
    parser_next_token(parser); /* consume union name */
    
    /* Expect opening brace */
    if (parser_current_token(parser) != '{') {
        parser_error(parser, (U8*)"Expected '{' after union name");
        return NULL;
    }
    parser_next_token(parser); /* consume '{' */
    
    /* Parse union members */
    ASTNode *members = NULL;
    I64 member_count = 0;
    
    while (parser_current_token(parser) != '}' && parser_current_token(parser) != TK_EOF) {
        /* Parse member declaration */
        if (parser_current_token(parser) != TK_IDENT) {
            parser_error(parser, (U8*)"Expected member type in union");
            if (members) ast_node_free(members);
            return NULL;
        }
        
        U8 *member_type = parser_current_token_value(parser);
        if (!member_type) {
            parser_error(parser, (U8*)"Failed to get member type");
            if (members) ast_node_free(members);
            return NULL;
        }
        parser_next_token(parser); /* consume member type */
        
        /* Parse member name */
        if (parser_current_token(parser) != TK_IDENT) {
            parser_error(parser, (U8*)"Expected member name after type");
            if (members) ast_node_free(members);
            return NULL;
        }
        
        U8 *member_name = parser_current_token_value(parser);
        if (!member_name) {
            parser_error(parser, (U8*)"Failed to get member name");
            if (members) ast_node_free(members);
            return NULL;
        }
        parser_next_token(parser); /* consume member name */
        
        /* Expect semicolon */
        if (parser_current_token(parser) != ';') {
            parser_error(parser, (U8*)"Expected ';' after member declaration");
            if (members) ast_node_free(members);
            return NULL;
        }
        parser_next_token(parser); /* consume ';' */
        
        /* Create member node */
        ASTNode *member_node = ast_node_new(NODE_VARIABLE, line, column);
        if (member_node) {
            member_node->data.variable.type = (void*)member_type;
            member_node->data.variable.name = strdup(member_name);
            
            /* Add to members list */
            if (!members) {
                members = member_node;
            } else {
                /* Add to end of list */
                ASTNode *current = members;
                while (current->next) {
                    current = current->next;
                }
                current->next = member_node;
            }
            member_count++;
        }
    }
    
    /* Expect closing brace */
    if (parser_current_token(parser) != '}') {
        parser_error(parser, (U8*)"Expected '}' after union members");
        if (members) ast_node_free(members);
        return NULL;
    }
    parser_next_token(parser); /* consume '}' */
    
    /* Expect semicolon */
    if (parser_current_token(parser) != ';') {
        parser_error(parser, (U8*)"Expected ';' after union definition");
        if (members) ast_node_free(members);
        return NULL;
    }
    parser_next_token(parser); /* consume ';' */
    
    /* Create type-prefixed union node */
    ASTNode *type_prefixed_union_node = ast_node_new(NODE_TYPE_PREFIXED_UNION, line, column);
    if (!type_prefixed_union_node) {
        if (members) ast_node_free(members);
        return NULL;
    }
    
    /* Initialize type-prefixed union data */
    type_prefixed_union_node->data.type_prefixed_union.prefix_type = strdup(prefix_type);
    type_prefixed_union_node->data.type_prefixed_union.union_name = strdup(union_name);
    type_prefixed_union_node->data.type_prefixed_union.members = members;
    type_prefixed_union_node->data.type_prefixed_union.member_count = member_count;
    type_prefixed_union_node->data.type_prefixed_union.is_public = is_public;
    
    printf("DEBUG: Type-prefixed union parsed successfully: %s %s, members: %lld, public: %s\n", 
           prefix_type, union_name, member_count, is_public ? "true" : "false");
    
    return type_prefixed_union_node;
}

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

void parser_initialize_builtin_functions(ParserState *parser) {
    if (!parser) return;
    
    /* Add Print function */
    ASTNode *print_func = ast_node_new(NODE_FUNCTION, 0, 0);
    if (print_func) {
        print_func->data.function.name = (U8*)"Print";
        print_func->data.function.return_type = (U8*)TK_TYPE_U0; /* void return type */
        print_func->data.function.parameters = NULL;
        print_func->data.function.body = NULL;
        print_func->data.function.is_extern = true;
        print_func->data.function.is_public = false;
        print_func->data.function.is_reg = false;
        print_func->data.function.is_interrupt = false;
        parser_add_symbol(parser, (U8*)"Print", print_func);
    }
    
    /* Add GetI64 function */
    ASTNode *geti64_func = ast_node_new(NODE_FUNCTION, 0, 0);
    if (geti64_func) {
        geti64_func->data.function.name = (U8*)"GetI64";
        geti64_func->data.function.return_type = (U8*)TK_TYPE_I64; /* I64 return type */
        geti64_func->data.function.parameters = NULL;
        geti64_func->data.function.body = NULL;
        geti64_func->data.function.is_extern = true;
        geti64_func->data.function.is_public = false;
        geti64_func->data.function.is_reg = false;
        geti64_func->data.function.is_interrupt = false;
        parser_add_symbol(parser, (U8*)"GetI64", geti64_func);
    }
    
    /* Add GetF64 function */
    ASTNode *getf64_func = ast_node_new(NODE_FUNCTION, 0, 0);
    if (getf64_func) {
        getf64_func->data.function.name = (U8*)"GetF64";
        getf64_func->data.function.return_type = (U8*)TK_TYPE_F64; /* F64 return type */
        getf64_func->data.function.parameters = NULL;
        getf64_func->data.function.body = NULL;
        getf64_func->data.function.is_extern = true;
        getf64_func->data.function.is_public = false;
        getf64_func->data.function.is_reg = false;
        getf64_func->data.function.is_interrupt = false;
        parser_add_symbol(parser, (U8*)"GetF64", getf64_func);
    }
    
    /* Add GetString function */
    ASTNode *getstring_func = ast_node_new(NODE_FUNCTION, 0, 0);
    if (getstring_func) {
        getstring_func->data.function.name = (U8*)"GetString";
        getstring_func->data.function.return_type = (U8*)TK_TYPE_I64; /* I64 return type (length) */
        getstring_func->data.function.parameters = NULL;
        getstring_func->data.function.body = NULL;
        getstring_func->data.function.is_extern = true;
        getstring_func->data.function.is_public = false;
        getstring_func->data.function.is_reg = false;
        getstring_func->data.function.is_interrupt = false;
        parser_add_symbol(parser, (U8*)"GetString", getstring_func);
    }
    
    /* Add PutChars function */
    ASTNode *putchars_func = ast_node_new(NODE_FUNCTION, 0, 0);
    if (putchars_func) {
        putchars_func->data.function.name = (U8*)"PutChars";
        putchars_func->data.function.return_type = (U8*)TK_TYPE_U0; /* void return type */
        putchars_func->data.function.parameters = NULL;
        putchars_func->data.function.body = NULL;
        putchars_func->data.function.is_extern = true;
        putchars_func->data.function.is_public = false;
        putchars_func->data.function.is_reg = false;
        putchars_func->data.function.is_interrupt = false;
        parser_add_symbol(parser, (U8*)"PutChars", putchars_func);
    }
    
    /* Add PutChar function */
    ASTNode *putchar_func = ast_node_new(NODE_FUNCTION, 0, 0);
    if (putchar_func) {
        putchar_func->data.function.name = (U8*)"PutChar";
        putchar_func->data.function.return_type = (U8*)TK_TYPE_U0; /* void return type */
        putchar_func->data.function.parameters = NULL;
        putchar_func->data.function.body = NULL;
        putchar_func->data.function.is_extern = true;
        putchar_func->data.function.is_public = false;
        putchar_func->data.function.is_reg = false;
        putchar_func->data.function.is_interrupt = false;
        parser_add_symbol(parser, (U8*)"PutChar", putchar_func);
    }
    
    printf("DEBUG: Initialized built-in functions in symbol table\n");
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

Bool parser_is_assembly_token(SchismTokenType token) {
    /* TODO: Implement assembly token checking */
    return false;
}

Bool parser_is_assembly_register_token(SchismTokenType token) {
    /* TODO: Implement assembly register token checking */
    return false;
}

Bool parser_is_assembly_opcode_token(SchismTokenType token) {
    /* TODO: Implement assembly opcode token checking */
    return false;
}

X86Register parser_get_assembly_register(SchismTokenType token, U8 *name) {
    /* TODO: Implement assembly register parsing */
    return X86_REG_NONE;
}

U8* parser_get_assembly_opcode(SchismTokenType token, U8 *name) {
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

ASTNode* parse_case_statement(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: Parsing case statement, current token: %d\n", parser_current_token(parser));
    
    I64 line = parser_current_line(parser);
    I64 column = parser_current_column(parser);
    
    /* Expect 'case' keyword */
    if (parser_current_token(parser) != TK_CASE) {
        parser_error(parser, (U8*)"Expected 'case' keyword");
        return NULL;
    }
    parser_next_token(parser); /* consume 'case' */
    
    /* Parse case value (optional - can be just 'case:') */
    ASTNode *value = NULL;
    ASTNode *range_start = NULL;
    ASTNode *range_end = NULL;
    Bool is_range = false;
    Bool is_null_case = false;
    I64 auto_increment_value = 0;
    
    if (parser_current_token(parser) != ':') {
        /* Parse case value */
        value = parse_expression(parser);
        if (!value) {
            parser_error(parser, (U8*)"Expected case value or ':'");
            return NULL;
        }
        
        /* Check for range expression (case 5...10:) */
        if (parser_current_token(parser) == TK_ELLIPSIS) {
            is_range = true;
            range_start = value;
            parser_next_token(parser); /* consume '...' */
            
            range_end = parse_expression(parser);
            if (!range_end) {
                parser_error(parser, (U8*)"Expected end value in range expression");
                ast_node_free(range_start);
                return NULL;
            }
        }
    } else {
        /* This is a null case (case:) - auto-increment from previous case */
        is_null_case = true;
        /* TODO: Calculate auto-increment value based on previous case */
        /* For now, we'll set it to 0 and handle it in the switch statement */
        auto_increment_value = 0;
    }
    
    /* Expect ':' */
    if (parser_current_token(parser) != ':') {
        parser_error(parser, (U8*)"Expected ':' after case value");
        if (value) ast_node_free(value);
        if (range_start) ast_node_free(range_start);
        if (range_end) ast_node_free(range_end);
        return NULL;
    }
    parser_next_token(parser); /* consume ':' */
    
    /* Parse case body statements */
    ASTNode *body = NULL;
    ASTNode *last_stmt = NULL;
    
    /* Parse statements until we hit another case, default, or closing brace */
    while (parser_current_token(parser) != TK_CASE && 
           parser_current_token(parser) != TK_DEFAULT && 
           parser_current_token(parser) != '}' && 
           parser_current_token(parser) != TK_EOF) {
        
        ASTNode *stmt = parse_statement(parser);
        if (!stmt) {
            parser_error(parser, (U8*)"Failed to parse statement in case body");
            if (body) ast_node_free(body);
            if (value) ast_node_free(value);
            if (range_start) ast_node_free(range_start);
            if (range_end) ast_node_free(range_end);
            return NULL;
        }
        
        /* Add to body list */
        if (!body) {
            body = stmt;
            last_stmt = stmt;
        } else {
            last_stmt->next = stmt;
            last_stmt = stmt;
        }
    }
    
    /* Create case statement node */
    ASTNode *case_node = ast_node_new(NODE_CASE, line, column);
    if (!case_node) {
        if (body) ast_node_free(body);
        if (value) ast_node_free(value);
        if (range_start) ast_node_free(range_start);
        if (range_end) ast_node_free(range_end);
        return NULL;
    }
    
    /* Initialize case statement data */
    case_node->data.case_stmt.value = value;
    case_node->data.case_stmt.range_start = range_start;
    case_node->data.case_stmt.range_end = range_end;
    case_node->data.case_stmt.body = body;
    case_node->data.case_stmt.is_range = is_range;
    case_node->data.case_stmt.is_default = false;
    case_node->data.case_stmt.is_null_case = is_null_case;
    case_node->data.case_stmt.auto_increment_value = auto_increment_value;
    
    printf("DEBUG: Case statement parsed successfully\n");
    return case_node;
}

ASTNode* parse_default_statement(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: Parsing default statement, current token: %d\n", parser_current_token(parser));
    
    I64 line = parser_current_line(parser);
    I64 column = parser_current_column(parser);
    
    /* Expect 'default' keyword */
    if (parser_current_token(parser) != TK_DEFAULT) {
        parser_error(parser, (U8*)"Expected 'default' keyword");
        return NULL;
    }
    parser_next_token(parser); /* consume 'default' */
    
    /* Expect ':' */
    if (parser_current_token(parser) != ':') {
        parser_error(parser, (U8*)"Expected ':' after 'default'");
        return NULL;
    }
    parser_next_token(parser); /* consume ':' */
    
    /* Parse default body statements */
    ASTNode *body = NULL;
    ASTNode *last_stmt = NULL;
    
    /* Parse statements until we hit another case, default, or closing brace */
    while (parser_current_token(parser) != TK_CASE && 
           parser_current_token(parser) != TK_DEFAULT && 
           parser_current_token(parser) != '}' && 
           parser_current_token(parser) != TK_EOF) {
        
        ASTNode *stmt = parse_statement(parser);
        if (!stmt) {
            parser_error(parser, (U8*)"Failed to parse statement in default body");
            if (body) ast_node_free(body);
            return NULL;
        }
        
        /* Add to body list */
        if (!body) {
            body = stmt;
            last_stmt = stmt;
        } else {
            last_stmt->next = stmt;
            last_stmt = stmt;
        }
    }
    
    /* Create case statement node (default is just a special case) */
    ASTNode *default_node = ast_node_new(NODE_CASE, line, column);
    if (!default_node) {
        if (body) ast_node_free(body);
        return NULL;
    }
    
    /* Initialize case statement data */
    default_node->data.case_stmt.value = NULL;
    default_node->data.case_stmt.range_start = NULL;
    default_node->data.case_stmt.range_end = NULL;
    default_node->data.case_stmt.body = body;
    default_node->data.case_stmt.is_range = false;
    default_node->data.case_stmt.is_default = true;
    
    printf("DEBUG: Default statement parsed successfully\n");
    return default_node;
}

ASTNode* parse_label_statement(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: Parsing label statement, current token: %d\n", parser_current_token(parser));
    
    I64 line = parser_current_line(parser);
    I64 column = parser_current_column(parser);
    
    /* Parse label name */
    if (parser_current_token(parser) != TK_IDENT) {
        parser_error(parser, (U8*)"Expected label name");
        return NULL;
    }
    
    U8 *label_name = parser_current_token_value(parser);
    if (!label_name) {
        parser_error(parser, (U8*)"Failed to get label name");
        return NULL;
    }
    parser_next_token(parser); /* consume label name */
    
    /* Check for exported label (label::) or local label (@@label:) */
    Bool is_exported = false;
    Bool is_local = false;
    
    if (parser_current_token(parser) == TK_DBL_COLON) {
        is_exported = true;
        parser_next_token(parser); /* consume '::' */
    } else if (parser_current_token(parser) == ':') {
        /* Check if this is a local label (@@label:) */
        if (label_name[0] == '@' && label_name[1] == '@') {
            is_local = true;
        }
        parser_next_token(parser); /* consume ':' */
    } else {
        parser_error(parser, (U8*)"Expected ':' or '::' after label name");
        return NULL;
    }
    
    /* Create label statement node */
    ASTNode *label_node = ast_node_new(NODE_LABEL, line, column);
    if (!label_node) {
        return NULL;
    }
    
    /* Initialize label statement data */
    label_node->data.label_stmt.label_name = label_name;
    label_node->data.label_stmt.is_exported = is_exported;
    label_node->data.label_stmt.is_local = is_local;
    label_node->data.label_stmt.label_address = 0; /* Will be set during codegen */
    
    printf("DEBUG: Label statement parsed successfully, name: %s, exported: %s, local: %s\n", 
           label_name, is_exported ? "true" : "false", is_local ? "true" : "false");
    return label_node;
}

ASTNode* parse_array_initializer(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: Parsing array initializer, current token: %d\n", parser_current_token(parser));
    
    I64 line = parser_current_line(parser);
    I64 column = parser_current_column(parser);
    
    /* Expect '{' */
    if (parser_current_token(parser) != '{') {
        parser_error(parser, (U8*)"Expected '{' for array initializer");
        return NULL;
    }
    parser_next_token(parser); /* consume '{' */
    
    /* Parse array elements */
    ASTNode *elements = NULL;
    ASTNode *last_element = NULL;
    I64 element_count = 0;
    
    /* Parse elements until we hit '}' */
    while (parser_current_token(parser) != '}' && parser_current_token(parser) != TK_EOF) {
        ASTNode *element = parse_expression(parser);
        if (!element) {
            parser_error(parser, (U8*)"Failed to parse array element");
            if (elements) ast_node_free(elements);
            return NULL;
        }
        
        /* Add to elements list */
        if (!elements) {
            elements = element;
            last_element = element;
        } else {
            last_element->next = element;
            last_element = element;
        }
        element_count++;
        
        /* Check for comma separator */
        if (parser_current_token(parser) == ',') {
            parser_next_token(parser); /* consume ',' */
        } else if (parser_current_token(parser) != '}') {
            parser_error(parser, (U8*)"Expected ',' or '}' in array initializer");
            if (elements) ast_node_free(elements);
            return NULL;
        }
    }
    
    /* Expect '}' */
    if (parser_current_token(parser) != '}') {
        parser_error(parser, (U8*)"Expected '}' to close array initializer");
        if (elements) ast_node_free(elements);
        return NULL;
    }
    parser_next_token(parser); /* consume '}' */
    
    /* Create array initializer node */
    ASTNode *array_init = ast_node_new(NODE_ARRAY_INIT, line, column);
    if (!array_init) {
        if (elements) ast_node_free(elements);
        return NULL;
    }
    
    /* Initialize array initializer data */
    array_init->data.array_init.elements = elements;
    array_init->data.array_init.element_count = element_count;
    
    printf("DEBUG: Array initializer parsed successfully with %ld elements\n", element_count);
    return array_init;
}

ASTNode* parse_pointer_dereference(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: Parsing pointer dereference, current token: %d\n", parser_current_token(parser));
    
    I64 line = parser_current_line(parser);
    I64 column = parser_current_column(parser);
    
    /* Expect '*' */
    if (parser_current_token(parser) != '*') {
        parser_error(parser, (U8*)"Expected '*' for pointer dereference");
        return NULL;
    }
    parser_next_token(parser); /* consume '*' */
    
    /* Parse the pointer expression */
    ASTNode *pointer = parse_primary_expression(parser);
    if (!pointer) {
        parser_error(parser, (U8*)"Expected pointer expression after '*'");
        return NULL;
    }
    
    /* Create pointer dereference node */
    ASTNode *deref_node = ast_node_new(NODE_POINTER_DEREF, line, column);
    if (!deref_node) {
        ast_node_free(pointer);
        return NULL;
    }
    
    /* Initialize pointer dereference data */
    deref_node->data.pointer_deref.pointer = pointer;
    
    printf("DEBUG: Pointer dereference parsed successfully\n");
    return deref_node;
}

ASTNode* parse_address_of(ParserState *parser) {
    if (!parser) return NULL;
    
    printf("DEBUG: Parsing address-of, current token: %d\n", parser_current_token(parser));
    
    I64 line = parser_current_line(parser);
    I64 column = parser_current_column(parser);
    
    /* Expect '&' */
    if (parser_current_token(parser) != '&') {
        parser_error(parser, (U8*)"Expected '&' for address-of");
        return NULL;
    }
    parser_next_token(parser); /* consume '&' */
    
    /* Parse the variable expression */
    ASTNode *variable = parse_primary_expression(parser);
    if (!variable) {
        parser_error(parser, (U8*)"Expected variable expression after '&'");
        return NULL;
    }
    
    /* Create address-of node */
    ASTNode *addr_node = ast_node_new(NODE_ADDRESS_OF, line, column);
    if (!addr_node) {
        ast_node_free(variable);
        return NULL;
    }
    
    /* Initialize address-of data */
    addr_node->data.address_of.variable = variable;
    
    printf("DEBUG: Address-of parsed successfully\n");
    return addr_node;
}

/*
 * Parse start/end block for sub-switch statements
 */
ASTNode* parse_start_end_block(ParserState *parser, Bool is_start) {
    if (!parser) return NULL;
    
    printf("DEBUG: Parsing %s block, current token: %d\n", is_start ? "start" : "end", parser_current_token(parser));
    
    I64 line = parser_current_line(parser);
    I64 column = parser_current_column(parser);
    
    /* Expect 'start' or 'end' keyword */
    SchismTokenType expected_token = is_start ? TK_START : TK_END;
    if (parser_current_token(parser) != expected_token) {
        parser_error(parser, (U8*)(is_start ? "Expected 'start' keyword" : "Expected 'end' keyword"));
        return NULL;
    }
    parser_next_token(parser); /* consume 'start' or 'end' */
    
    /* Expect ':' */
    if (parser_current_token(parser) != ':') {
        parser_error(parser, (U8*)"Expected ':' after start/end keyword");
        return NULL;
    }
    parser_next_token(parser); /* consume ':' */
    
    /* Parse statements in the block */
    ASTNode *statements = NULL;
    ASTNode *last_stmt = NULL;
    
    /* Parse statements until we hit another case, default, start, end, or closing brace */
    while (parser_current_token(parser) != TK_CASE && 
           parser_current_token(parser) != TK_DEFAULT && 
           parser_current_token(parser) != TK_START && 
           parser_current_token(parser) != TK_END && 
           parser_current_token(parser) != '}' && 
           parser_current_token(parser) != TK_EOF) {
        
        ASTNode *stmt = parse_statement(parser);
        if (!stmt) {
            parser_error(parser, (U8*)"Failed to parse statement in start/end block");
            if (statements) ast_node_free(statements);
            return NULL;
        }
        
        /* Add to statements list */
        if (!statements) {
            statements = stmt;
            last_stmt = stmt;
        } else {
            last_stmt->next = stmt;
            last_stmt = stmt;
        }
    }
    
    /* Create start/end block node */
    ASTNode *block_node = ast_node_new(is_start ? NODE_START_BLOCK : NODE_END_BLOCK, line, column);
    if (!block_node) {
        if (statements) ast_node_free(statements);
        return NULL;
    }
    
    /* Initialize start/end block data */
    block_node->data.start_end_block.statements = statements;
    block_node->data.start_end_block.is_start = is_start;
    
    printf("DEBUG: %s block parsed successfully\n", is_start ? "Start" : "End");
    return block_node;
}

/*
 * Parse simple expressions (identifiers and constants only) to avoid circular dependencies
 */
ASTNode* parse_simple_expression(ParserState *parser) {
    if (!parser) return NULL;
    
    SchismTokenType current = parser_current_token(parser);
    I64 line = parser_current_line(parser);
    I64 column = parser_current_column(parser);
    
    switch (current) {
        case TK_IDENT: {
            /* Identifier */
            ASTNode *node = ast_node_new(NODE_IDENTIFIER, line, column);
            if (!node) return NULL;
            
            U8 *name = parser_current_token_value(parser);
            if (name) {
                I64 len = strlen((char*)name);
                node->data.identifier.name = (U8*)malloc(len + 1);
                if (node->data.identifier.name) {
                    strcpy((char*)node->data.identifier.name, (char*)name);
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
        
        case TK_TRUE:
        case TK_FALSE: {
            /* Boolean literal */
            Bool bool_value = (current == TK_TRUE);
            parser_next_token(parser);
            
            ASTNode *node = ast_node_new(NODE_BOOLEAN, line, column);
            if (!node) return NULL;
            
            node->data.boolean.value = bool_value;
            return node;
        }
        
        default:
            return NULL;
    }
}

/*
 * Parse range comparison expressions (5<i<j+1<20)
 */
ASTNode* parse_range_comparison(ParserState *parser, ASTNode *first_expr) {
    if (!parser || !first_expr) return NULL;
    
    printf("DEBUG: parse_range_comparison - starting, current token: %d\n", parser_current_token(parser));
    
    I64 line = parser_current_line(parser);
    I64 column = parser_current_column(parser);
    
    printf("DEBUG: parse_range_comparison - using provided first expression\n");
    printf("DEBUG: parse_range_comparison - first_expr type: %d\n", first_expr->type);
    
    /* Collect all expressions and operators in the range */
    ASTNode *expressions = first_expr;
    ASTNode *operators = NULL;
    ASTNode *last_expr = first_expr;
    ASTNode *last_op = NULL;
    I64 expression_count = 1;
    
    /* Parse the range chain */
    printf("DEBUG: parse_range_comparison - entering while loop, current token: %d\n", parser_current_token(parser));
    while (parser_current_token(parser) == '<' || parser_current_token(parser) == '>' ||
           parser_current_token(parser) == TK_LESS_EQU || parser_current_token(parser) == TK_GREATER_EQU) {
        printf("DEBUG: parse_range_comparison - in while loop, current token: %d\n", parser_current_token(parser));
        
        /* Get the comparison operator */
        SchismTokenType op = parser_current_token(parser);
        parser_next_token(parser); /* consume operator */
        
        /* Create operator node */
        ASTNode *op_node = ast_node_new(NODE_BINARY_OP, parser_current_line(parser), parser_current_column(parser));
        if (!op_node) {
            ast_node_free(expressions);
            if (operators) ast_node_free(operators);
            return NULL;
        }
        
        /* Set operator type */
        switch (op) {
            case '<': op_node->data.binary_op.op = BINOP_LT; break;
            case '>': op_node->data.binary_op.op = BINOP_GT; break;
            case TK_LESS_EQU: op_node->data.binary_op.op = BINOP_LE; break;
            case TK_GREATER_EQU: op_node->data.binary_op.op = BINOP_GE; break;
            default: op_node->data.binary_op.op = BINOP_LT; break;
        }
        
        /* Add operator to operators list */
        if (!operators) {
            operators = op_node;
            last_op = op_node;
        } else {
            last_op->next = op_node;
            last_op = op_node;
        }
        
        /* Parse the next expression - use simple identifier/constant to avoid circular dependency */
        ASTNode *next_expr = parse_simple_expression(parser);
        if (!next_expr) {
            parser_error(parser, (U8*)"Expected expression after comparison operator in range");
            ast_node_free(expressions);
            ast_node_free(operators);
            return NULL;
        }
        
        /* Add expression to expressions list */
        last_expr->next = next_expr;
        last_expr = next_expr;
        expression_count++;
    }
    
    /* Create range comparison node */
    ASTNode *range_node = ast_node_new(NODE_RANGE_COMPARISON, line, column);
    if (!range_node) {
        ast_node_free(expressions);
        ast_node_free(operators);
        return NULL;
    }
    
    /* Initialize range comparison data */
    range_node->data.range_comparison.expressions = expressions;
    range_node->data.range_comparison.operators = operators;
    range_node->data.range_comparison.expression_count = expression_count;
    
    printf("DEBUG: Range comparison parsed successfully with %ld expressions\n", expression_count);
    return range_node;
}

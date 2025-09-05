#include "holyc.h"

// Parser functions

ASTNode* create_node(NodeType type) {
    ASTNode* node = MAlloc(sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = type;
    node->value.int_val = 0;
    node->left = NULL;
    node->right = NULL;
    node->children = NULL;
    node->next = NULL;
    node->name = NULL;
    node->line_num = 0;
    node->col_num = 0;
    
    return node;
}

void free_node(ASTNode* node) {
    if (!node) return;
    
    free_node(node->left);
    free_node(node->right);
    free_node(node->children);
    free_node(node->next);
    
    if (node->name) {
        Free(node->name);
    }
    
    Free(node);
}

void parser_cleanup(ASTNode* node) {
    free_node(node);
}

Symbol* add_symbol(CompilerContext* ctx, const char* name, I64 type, I64 size) {
    Symbol* sym = MAlloc(sizeof(Symbol));
    if (!sym) return NULL;
    
    sym->name = StrNew(name);
    sym->type = type;
    sym->size = size;
    sym->offset = 0;
    sym->is_function = false;
    sym->is_global = true;
    sym->next = ctx->symbol_table;
    ctx->symbol_table = sym;
    
    return sym;
}

Symbol* find_symbol(CompilerContext* ctx, const char* name) {
    Symbol* sym = ctx->symbol_table;
    while (sym) {
        if (strcmp(sym->name, name) == 0) {
            return sym;
        }
        sym = sym->next;
    }
    return NULL;
}

void error(CompilerContext* ctx, const char* message, ...) {
    va_list args;
    va_start(args, message);
    fprintf(stderr, "%s:%lld:%lld: error: ", ctx->filename, ctx->line_num, ctx->col_num);
    vfprintf(stderr, message, args);
    fprintf(stderr, "\n");
    va_end(args);
    ctx->error_count++;
}

void warning(CompilerContext* ctx, const char* message, ...) {
    va_list args;
    va_start(args, message);
    fprintf(stderr, "%s:%lld:%lld: warning: ", ctx->filename, ctx->line_num, ctx->col_num);
    vfprintf(stderr, message, args);
    fprintf(stderr, "\n");
    va_end(args);
    ctx->warning_count++;
}

ASTNode* parse_program(CompilerContext* ctx) {
    ASTNode* program = create_node(NODE_PROGRAM);
    ASTNode* current = NULL;
    
    while (ctx->current_token != TK_EOF) {
        ASTNode* stmt = parse_statement(ctx);
        if (stmt) {
            if (!program->children) {
                program->children = stmt;
                current = stmt;
            } else {
                current->next = stmt;
                current = stmt;
            }
        }
    }
    return program;
}

ASTNode* parse_statement(CompilerContext* ctx) {
    switch (ctx->current_token) {
        case TK_IF:
            return parse_if_statement(ctx);
        case TK_WHILE:
            return parse_while_statement(ctx);
        case TK_FOR:
            return parse_for_statement(ctx);
        case TK_RETURN:
            return parse_return_statement(ctx);
        case TK_BREAK:
            return parse_break_statement(ctx);
        case TK_CONTINUE:
            return parse_continue_statement(ctx);
        case TK_GOTO:
            return parse_goto_statement(ctx);
        case TK_ASM:
            return parse_asm_statement(ctx);
        case TK_CLASS:
            return parse_class_definition(ctx);
        case TK_STR:
            // In HolyC, a string literal by itself is treated as Print(string)
            return parse_print_statement(ctx);
        case TK_IDENT:
            return parse_expression_statement(ctx);
        case ';':
            return create_node(NODE_PROGRAM); // Empty statement
        default:
            return parse_expression_statement(ctx);
    }
}

ASTNode* parse_print_statement(CompilerContext* ctx) {
    // Create a function call node for Print()
    ASTNode* call = create_node(NODE_CALL);
    call->name = StrNew("Print");
    
    // Create argument node for the string literal
    ASTNode* arg = create_node(NODE_STRING);
    arg->name = StrNew(ctx->current_string);
    
    // Set up the call structure
    call->children = arg;
    
    // Consume the string token and semicolon
    lex_next_token(ctx); // consume TK_STR
    if (ctx->current_token == ';') {
        lex_next_token(ctx); // consume ';'
    }
    
    return call;
}

ASTNode* parse_expression_statement(CompilerContext* ctx) {
    ASTNode* expr = parse_expression(ctx);
    
    if (ctx->current_token == ';') {
        lex_next_token(ctx);
    }
    
    return expr;
}

ASTNode* parse_expression(CompilerContext* ctx) {
    return parse_assignment_expression(ctx);
}

ASTNode* parse_assignment_expression(CompilerContext* ctx) {
    ASTNode* left = parse_logical_or_expression(ctx);
    
    if (ctx->current_token == '=' || 
        ctx->current_token == TK_ADD_EQU ||
        ctx->current_token == TK_SUB_EQU ||
        ctx->current_token == TK_MUL_EQU ||
        ctx->current_token == TK_DIV_EQU ||
        ctx->current_token == TK_MOD_EQU ||
        ctx->current_token == TK_AND_EQU ||
        ctx->current_token == TK_OR_EQU ||
        ctx->current_token == TK_XOR_EQU ||
        ctx->current_token == TK_SHL_EQU ||
        ctx->current_token == TK_SHR_EQU) {
        
        ASTNode* node = create_node(NODE_ASSIGNMENT);
        node->value.binary_op = (BinaryOp)(ctx->current_token - '=');
        node->left = left;
        lex_next_token(ctx);
        node->right = parse_assignment_expression(ctx);
        return node;
    }
    
    return left;
}

ASTNode* parse_logical_or_expression(CompilerContext* ctx) {
    ASTNode* left = parse_logical_and_expression(ctx);
    
    while (ctx->current_token == TK_OR_OR) {
        ASTNode* node = create_node(NODE_BINARY_OP);
        node->value.binary_op = OP_OR_OR;
        node->left = left;
        lex_next_token(ctx);
        node->right = parse_logical_and_expression(ctx);
        left = node;
    }
    
    return left;
}

ASTNode* parse_logical_and_expression(CompilerContext* ctx) {
    ASTNode* left = parse_equality_expression(ctx);
    
    while (ctx->current_token == TK_AND_AND) {
        ASTNode* node = create_node(NODE_BINARY_OP);
        node->value.binary_op = OP_AND_AND;
        node->left = left;
        lex_next_token(ctx);
        node->right = parse_equality_expression(ctx);
        left = node;
    }
    
    return left;
}

ASTNode* parse_equality_expression(CompilerContext* ctx) {
    ASTNode* left = parse_relational_expression(ctx);
    
    while (ctx->current_token == TK_EQU_EQU || ctx->current_token == TK_NOT_EQU) {
        ASTNode* node = create_node(NODE_BINARY_OP);
        node->value.binary_op = (ctx->current_token == TK_EQU_EQU) ? OP_EQU : OP_NE;
        node->left = left;
        lex_next_token(ctx);
        node->right = parse_relational_expression(ctx);
        left = node;
    }
    
    return left;
}

ASTNode* parse_relational_expression(CompilerContext* ctx) {
    ASTNode* left = parse_shift_expression(ctx);
    
    while (ctx->current_token == '<' || ctx->current_token == '>' ||
           ctx->current_token == TK_LESS_EQU || ctx->current_token == TK_GREATER_EQU) {
        ASTNode* node = create_node(NODE_BINARY_OP);
        switch (ctx->current_token) {
            case '<': node->value.binary_op = OP_LT; break;
            case '>': node->value.binary_op = OP_GT; break;
            case TK_LESS_EQU: node->value.binary_op = OP_LE; break;
            case TK_GREATER_EQU: node->value.binary_op = OP_GE; break;
        }
        node->left = left;
        lex_next_token(ctx);
        node->right = parse_shift_expression(ctx);
        left = node;
    }
    
    return left;
}

ASTNode* parse_shift_expression(CompilerContext* ctx) {
    ASTNode* left = parse_additive_expression(ctx);
    
    while (ctx->current_token == TK_SHL || ctx->current_token == TK_SHR) {
        ASTNode* node = create_node(NODE_BINARY_OP);
        node->value.binary_op = (ctx->current_token == TK_SHL) ? OP_SHL : OP_SHR;
        node->left = left;
        lex_next_token(ctx);
        node->right = parse_additive_expression(ctx);
        left = node;
    }
    
    return left;
}

ASTNode* parse_additive_expression(CompilerContext* ctx) {
    ASTNode* left = parse_multiplicative_expression(ctx);
    
    while (ctx->current_token == '+' || ctx->current_token == '-') {
        ASTNode* node = create_node(NODE_BINARY_OP);
        node->value.binary_op = (ctx->current_token == '+') ? OP_ADD : OP_SUB;
        node->left = left;
        lex_next_token(ctx);
        node->right = parse_multiplicative_expression(ctx);
        left = node;
    }
    
    return left;
}

ASTNode* parse_multiplicative_expression(CompilerContext* ctx) {
    ASTNode* left = parse_unary_expression(ctx);
    
    while (ctx->current_token == '*' || ctx->current_token == '/' || ctx->current_token == '%') {
        ASTNode* node = create_node(NODE_BINARY_OP);
        switch (ctx->current_token) {
            case '*': node->value.binary_op = OP_MUL; break;
            case '/': node->value.binary_op = OP_DIV; break;
            case '%': node->value.binary_op = OP_MOD; break;
        }
        node->left = left;
        lex_next_token(ctx);
        node->right = parse_unary_expression(ctx);
        left = node;
    }
    
    return left;
}

ASTNode* parse_unary_expression(CompilerContext* ctx) {
    if (ctx->current_token == '+' || ctx->current_token == '-' ||
        ctx->current_token == '!' || ctx->current_token == '~' ||
        ctx->current_token == '*' || ctx->current_token == '&') {
        
        ASTNode* node = create_node(NODE_UNARY_OP);
        switch (ctx->current_token) {
            case '+': node->value.unary_op = OP_PLUS; break;
            case '-': node->value.unary_op = OP_MINUS; break;
            case '!': node->value.unary_op = OP_NOT; break;
            case '~': node->value.unary_op = OP_COMP; break;
            case '*': node->value.unary_op = OP_DEREF; break;
            case '&': node->value.unary_op = OP_ADDR; break;
        }
        lex_next_token(ctx);
        node->left = parse_unary_expression(ctx);
        return node;
    }
    
    return parse_postfix_expression(ctx);
}

ASTNode* parse_postfix_expression(CompilerContext* ctx) {
    ASTNode* left = parse_primary_expression(ctx);
    
    while (1) {
        if (ctx->current_token == '[') {
            // Array access
            ASTNode* node = create_node(NODE_ARRAY_ACCESS);
            node->left = left;
            lex_next_token(ctx);
            node->right = parse_expression(ctx);
            if (ctx->current_token != ']') {
                error(ctx, "Expected ']'");
            }
            lex_next_token(ctx);
            left = node;
        } else if (ctx->current_token == '.') {
            // Member access
            ASTNode* node = create_node(NODE_MEMBER_ACCESS);
            node->left = left;
            lex_next_token(ctx);
            if (ctx->current_token != TK_IDENT) {
                error(ctx, "Expected identifier after '.'");
            }
            node->right = create_node(NODE_IDENTIFIER);
            node->right->name = StrNew(ctx->current_string);
            lex_next_token(ctx);
            left = node;
        } else if (ctx->current_token == '(') {
            // Function call
            ASTNode* node = create_node(NODE_CALL);
            node->left = left;
            lex_next_token(ctx);
            
            // Parse arguments
            ASTNode* args = NULL;
            ASTNode* current_arg = NULL;
            
            if (ctx->current_token != ')') {
                args = parse_expression(ctx);
                current_arg = args;
                
                while (ctx->current_token == ',') {
                    lex_next_token(ctx);
                    ASTNode* arg = parse_expression(ctx);
                    if (current_arg) {
                        current_arg->next = arg;
                        current_arg = arg;
                    } else {
                        args = arg;
                        current_arg = arg;
                    }
                }
            }
            
            node->right = args;
            
            if (ctx->current_token != ')') {
                error(ctx, "Expected ')'");
            }
            lex_next_token(ctx);
            left = node;
        } else {
            break;
        }
    }
    
    return left;
}

ASTNode* parse_primary_expression(CompilerContext* ctx) {
    switch (ctx->current_token) {
        case TK_I64: {
            ASTNode* node = create_node(NODE_NUMBER);
            node->value.int_val = ctx->current_int;
            lex_next_token(ctx);
            return node;
        }
        case TK_F64: {
            ASTNode* node = create_node(NODE_NUMBER);
            node->value.float_val = ctx->current_float;
            lex_next_token(ctx);
            return node;
        }
        case TK_STR: {
            ASTNode* node = create_node(NODE_STRING);
            node->name = StrNew(ctx->current_string);
            lex_next_token(ctx);
            return node;
        }
        case TK_CHAR_CONST: {
            ASTNode* node = create_node(NODE_CHAR_CONST);
            node->value.int_val = ctx->current_int;
            lex_next_token(ctx);
            return node;
        }
        case TK_IDENT: {
            ASTNode* node = create_node(NODE_IDENTIFIER);
            node->name = StrNew(ctx->current_string);
            lex_next_token(ctx);
            return node;
        }
        case '(': {
            lex_next_token(ctx);
            ASTNode* expr = parse_expression(ctx);
            if (ctx->current_token != ')') {
                error(ctx, "Expected ')'");
            }
            lex_next_token(ctx);
            return expr;
        }
        default:
            error(ctx, "Unexpected token");
            return create_node(NODE_PROGRAM);
    }
}

// Simplified statement parsers
ASTNode* parse_if_statement(CompilerContext* ctx) {
    ASTNode* node = create_node(NODE_IF);
    lex_next_token(ctx); // Skip 'if'
    
    if (ctx->current_token != '(') {
        error(ctx, "Expected '(' after 'if'");
    }
    lex_next_token(ctx);
    
    node->left = parse_expression(ctx);
    
    if (ctx->current_token != ')') {
        error(ctx, "Expected ')' after 'if' condition");
    }
    lex_next_token(ctx);
    
    node->right = parse_statement(ctx);
    
    if (ctx->current_token == TK_ELSE) {
        lex_next_token(ctx);
        ASTNode* else_node = create_node(NODE_IF);
        else_node->left = node->right;
        else_node->right = parse_statement(ctx);
        node->right = else_node;
    }
    
    return node;
}

ASTNode* parse_while_statement(CompilerContext* ctx) {
    ASTNode* node = create_node(NODE_WHILE);
    lex_next_token(ctx); // Skip 'while'
    
    if (ctx->current_token != '(') {
        error(ctx, "Expected '(' after 'while'");
    }
    lex_next_token(ctx);
    
    node->left = parse_expression(ctx);
    
    if (ctx->current_token != ')') {
        error(ctx, "Expected ')' after 'while' condition");
    }
    lex_next_token(ctx);
    
    node->right = parse_statement(ctx);
    
    return node;
}

ASTNode* parse_for_statement(CompilerContext* ctx) {
    ASTNode* node = create_node(NODE_FOR);
    lex_next_token(ctx); // Skip 'for'
    
    if (ctx->current_token != '(') {
        error(ctx, "Expected '(' after 'for'");
    }
    lex_next_token(ctx);
    
    // Parse initialization
    node->left = parse_expression(ctx);
    
    if (ctx->current_token != ';') {
        error(ctx, "Expected ';' after 'for' initialization");
    }
    lex_next_token(ctx);
    
    // Parse condition
    node->right = parse_expression(ctx);
    
    if (ctx->current_token != ';') {
        error(ctx, "Expected ';' after 'for' condition");
    }
    lex_next_token(ctx);
    
    // Parse increment
    ASTNode* increment = parse_expression(ctx);
    
    if (ctx->current_token != ')') {
        error(ctx, "Expected ')' after 'for' increment");
    }
    lex_next_token(ctx);
    
    // Parse body
    ASTNode* body = parse_statement(ctx);
    
    // Create a more complex structure for for loops
    ASTNode* for_body = create_node(NODE_FOR);
    for_body->left = increment;
    for_body->right = body;
    
    node->children = for_body;
    
    return node;
}

ASTNode* parse_return_statement(CompilerContext* ctx) {
    ASTNode* node = create_node(NODE_RETURN);
    lex_next_token(ctx); // Skip 'return'
    
    if (ctx->current_token != ';') {
        node->left = parse_expression(ctx);
    }
    
    return node;
}

ASTNode* parse_break_statement(CompilerContext* ctx) {
    ASTNode* node = create_node(NODE_BREAK);
    lex_next_token(ctx); // Skip 'break'
    return node;
}

ASTNode* parse_continue_statement(CompilerContext* ctx) {
    ASTNode* node = create_node(NODE_CONTINUE);
    lex_next_token(ctx); // Skip 'continue'
    return node;
}

ASTNode* parse_goto_statement(CompilerContext* ctx) {
    ASTNode* node = create_node(NODE_GOTO);
    lex_next_token(ctx); // Skip 'goto'
    
    if (ctx->current_token != TK_IDENT) {
        error(ctx, "Expected identifier after 'goto'");
    }
    
    node->name = StrNew(ctx->current_string);
    lex_next_token(ctx);
    
    return node;
}

ASTNode* parse_asm_statement(CompilerContext* ctx) {
    ASTNode* node = create_node(NODE_ASM);
    lex_next_token(ctx); // Skip 'asm'
    
    if (ctx->current_token != '{') {
        error(ctx, "Expected '{' after 'asm'");
    }
    lex_next_token(ctx);
    
    // Parse assembly code (simplified)
    String asm_code = StrNew("");
    while (ctx->current_token != '}' && ctx->current_token != TK_EOF) {
        if (ctx->current_token == TK_STR) {
            String temp = StrPrint("%s%s", asm_code, ctx->current_string);
            Free(asm_code);
            asm_code = temp;
        }
        lex_next_token(ctx);
    }
    
    if (ctx->current_token != '}') {
        error(ctx, "Expected '}' after assembly code");
    }
    lex_next_token(ctx);
    
    node->name = asm_code;
    return node;
}

ASTNode* parse_class_definition(CompilerContext* ctx) {
    ASTNode* node = create_node(NODE_CLASS);
    lex_next_token(ctx); // Skip 'class'
    
    if (ctx->current_token != TK_IDENT) {
        error(ctx, "Expected class name");
    }
    
    node->name = StrNew(ctx->current_string);
    lex_next_token(ctx);
    
    if (ctx->current_token != '{') {
        error(ctx, "Expected '{' after class name");
    }
    lex_next_token(ctx);
    
    // Parse class members (simplified)
    while (ctx->current_token != '}' && ctx->current_token != TK_EOF) {
        // Skip members for now
        lex_next_token(ctx);
    }
    
    if (ctx->current_token != '}') {
        error(ctx, "Expected '}' after class definition");
    }
    lex_next_token(ctx);
    
    return node;
}

/*
 * Simple Lexer for SchismC - Initial Implementation
 * This is a minimal lexer for testing the build system
 */

#include "lexer.h"
#include "core_structures.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Simple lexer state */
typedef struct {
    FILE *input_file;
    SchismTokenType current_token;
    U8 *token_value;
    I64 line;
    I64 column;
} SimpleLexerState;

/* Function implementations - minimal versions */

LexerState* lexer_new(FILE *input) {
    LexerState *lexer = (LexerState*)malloc(sizeof(LexerState));
    if (!lexer) return NULL;
    
    memset(lexer, 0, sizeof(LexerState));
    lexer->input_file = input;
    lexer->current_token = TK_EOF;
    lexer->buffer_line = 1;
    lexer->buffer_column = 1;
    
    return lexer;
}

void lexer_free(LexerState *lexer) {
    if (!lexer) return;
    
    if (lexer->input_buffer) free(lexer->input_buffer);
    if (lexer->token_value) free(lexer->token_value);
    if (lexer->last_error) free(lexer->last_error);
    if (lexer->char_bitmap) free(lexer->char_bitmap);
    
    free(lexer);
}

SchismTokenType lex_next_token(LexerState *lexer) {
    if (!lexer || !lexer->input_file) return TK_EOF;
    
    int c = fgetc(lexer->input_file);
    if (c == EOF) {
        lexer->current_token = TK_EOF;
        return TK_EOF;
    }
    
    /* Simple token recognition */
    switch (c) {
        case '(': case ')': case '{': case '}': case '[': case ']':
        case ';': case ',': case ':': case '?': case '~':
        case '+': case '-': case '*': case '/': case '%':
        case '=': case '!': case '<': case '>': case '&': case '|': case '^':
            lexer->current_token = c;
            lexer->buffer_column++;
            return c;
            
        case '"':
            lexer->current_token = TK_STR;
            lexer->token_value = lex_create_string((U8*)"\"hello\"", 7);
            lexer->buffer_column++;
            return TK_STR;
            
        case '\'':
            lexer->current_token = TK_CHAR_CONST;
            lexer->token_value = lex_create_string((U8*)"'a'", 3);
            lexer->buffer_column++;
            return TK_CHAR_CONST;
            
        case '\n':
            lexer->buffer_line++;
            lexer->buffer_column = 1;
            return lex_next_token(lexer);  /* Skip newlines */
            
        case ' ':
        case '\t':
        case '\r':
            lexer->buffer_column++;
            return lex_next_token(lexer);  /* Skip whitespace */
            
        default:
            if (isdigit(c)) {
                lexer->current_token = TK_I64;
                lexer->token_value = lex_create_string((U8*)"42", 2);
                lexer->buffer_column++;
                return TK_I64;
            } else if (isalpha(c) || c == '_') {
                lexer->current_token = TK_IDENT;
                lexer->token_value = lex_create_string((U8*)"ident", 5);
                lexer->buffer_column++;
                return TK_IDENT;
            } else {
                lexer->current_token = c;
                lexer->buffer_column++;
                return c;
            }
    }
}

SchismTokenType lex_peek_token(LexerState *lexer) {
    if (!lexer) return TK_EOF;
    
    long pos = ftell(lexer->input_file);
    SchismTokenType token = lex_next_token(lexer);
    fseek(lexer->input_file, pos, SEEK_SET);
    return token;
}

U8* lex_get_token_value(LexerState *lexer) {
    return lexer ? lexer->token_value : NULL;
}

I64 lex_get_token_line(LexerState *lexer) {
    return lexer ? lexer->buffer_line : 0;
}

I64 lex_get_token_column(LexerState *lexer) {
    return lexer ? lexer->buffer_column : 0;
}

/* Assembly functions - minimal implementations */
Bool lex_is_assembly_register(U8 *str) {
    return false;  /* TODO: Implement */
}

X86Register lex_parse_register(U8 *str) {
    return REG_NONE;  /* TODO: Implement */
}

I64 lex_parse_operand_size(U8 *str) {
    return 0;  /* TODO: Implement */
}

Bool lex_is_assembly_opcode(U8 *str) {
    return false;  /* TODO: Implement */
}

/* HolyC functions - minimal implementations */
Bool lex_is_holyc_keyword(U8 *str) {
    return false;  /* TODO: Implement */
}

Bool lex_is_builtin_type(U8 *str) {
    return false;  /* TODO: Implement */
}

SchismTokenType lex_get_builtin_type_token(U8 *str) {
    return TK_IDENT;  /* TODO: Implement */
}

/* Character classification */
Bool lex_is_alpha(U8 c) {
    return isalpha(c);
}

Bool lex_is_digit(U8 c) {
    return isdigit(c);
}

Bool lex_is_alnum(U8 c) {
    return isalnum(c);
}

Bool lex_is_whitespace(U8 c) {
    return isspace(c);
}

Bool lex_is_newline(U8 c) {
    return (c == '\n' || c == '\r');
}

/* String handling */
U8* lex_create_string(U8 *str, I64 len) {
    if (!str || len <= 0) return NULL;
    
    U8 *result = (U8*)malloc(len + 1);
    if (!result) return NULL;
    
    memcpy(result, str, len);
    result[len] = '\0';
    
    return result;
}

void lex_free_string(U8 *str) {
    if (str) free(str);
}

/* Error handling */
void lex_error(LexerState *lexer, const char *message) {
    if (!lexer || !message) return;
    
    lexer->error_count++;
    if (lexer->last_error) free(lexer->last_error);
    lexer->last_error = lex_create_string((U8*)message, strlen(message));
    
    fprintf(stderr, "Lexer Error at line %lld: %s\n", 
            lexer->buffer_line, message);
}

void lex_warning(LexerState *lexer, const char *message) {
    if (!lexer || !message) return;
    
    lexer->warning_count++;
    fprintf(stderr, "Lexer Warning at line %lld: %s\n", 
            lexer->buffer_line, message);
}


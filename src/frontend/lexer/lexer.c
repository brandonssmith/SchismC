/*
 * Enhanced Lexer Implementation for SchismC
 * Ported from TempleOS HolyC with assembly-influenced enhancements
 */

#include "lexer.h"
#include "core_structures.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/* Forward declarations */
static TokenType lex_parse_string(LexerState *lexer);
static TokenType lex_parse_char(LexerState *lexer);
static TokenType lex_parse_number(LexerState *lexer);
static TokenType lex_parse_identifier(LexerState *lexer);

/* Keyword lookup table */
typedef struct {
    const char *name;
    TokenType token;
} Keyword;

static Keyword keywords[] = {
    /* Control flow */
    {"if", TK_IF},
    {"else", TK_ELSE},
    {"while", TK_WHILE},
    {"for", TK_FOR},
    {"do", TK_DO},
    {"switch", TK_SWITCH},
    {"case", TK_CASE},
    {"default", TK_DEFAULT},
    {"break", TK_BREAK},
    {"continue", TK_CONTINUE},
    {"return", TK_RETURN},
    {"goto", TK_GOTO},
    
    /* Preprocessor */
    {"ifdef", TK_IFDEF},
    {"ifndef", TK_IFNDEF},
    {"endif", TK_ENDIF},
    {"define", TK_DEFINE},
    {"include", TK_INCLUDE},
    
    /* Assembly */
    {"asm", TK_ASM},
    {"reg", TK_REG},
    {"noreg", TK_NOREG},
    {"interrupt", TK_INTERRUPT},
    {"haserrcode", TK_HASERRCODE},
    {"argpop", TK_ARGPOP},
    {"noargpop", TK_NOARGPOP},
    
    /* Type system */
    {"class", TK_CLASS},
    {"union", TK_UNION},
    {"public", TK_PUBLIC},
    {"extern", TK_EXTERN},
    {"import", TK_IMPORT},
    {"lastclass", TK_LASTCLASS},
    
    /* Exception handling */
    {"try", TK_TRY},
    {"catch", TK_CATCH},
    {"throw", TK_THROW},
    {"no_warn", TK_NO_WARN},
    
    /* Built-in types */
    {"I0", TK_TYPE_I0},
    {"I8", TK_TYPE_I8},
    {"I16", TK_TYPE_I16},
    {"I32", TK_TYPE_I32},
    {"I64", TK_TYPE_I64},
    {"U0", TK_TYPE_U0},
    {"U8", TK_TYPE_U8},
    {"U16", TK_TYPE_U16},
    {"U32", TK_TYPE_U32},
    {"U64", TK_TYPE_U64},
    {"F32", TK_TYPE_F32},
    {"F64", TK_TYPE_F64},
    {"Bool", TK_TYPE_BOOL},
    {"String", TK_TYPE_STRING},
    
    /* Assembly registers */
    {"RAX", TK_ASM_REG}, {"RCX", TK_ASM_REG}, {"RDX", TK_ASM_REG}, {"RBX", TK_ASM_REG},
    {"RSP", TK_ASM_REG}, {"RBP", TK_ASM_REG}, {"RSI", TK_ASM_REG}, {"RDI", TK_ASM_REG},
    {"R8", TK_ASM_REG}, {"R9", TK_ASM_REG}, {"R10", TK_ASM_REG}, {"R11", TK_ASM_REG},
    {"R12", TK_ASM_REG}, {"R13", TK_ASM_REG}, {"R14", TK_ASM_REG}, {"R15", TK_ASM_REG},
    {"EAX", TK_ASM_REG}, {"ECX", TK_ASM_REG}, {"EDX", TK_ASM_REG}, {"EBX", TK_ASM_REG},
    {"ESP", TK_ASM_REG}, {"EBP", TK_ASM_REG}, {"ESI", TK_ASM_REG}, {"EDI", TK_ASM_REG},
    {"AX", TK_ASM_REG}, {"CX", TK_ASM_REG}, {"DX", TK_ASM_REG}, {"BX", TK_ASM_REG},
    {"SP", TK_ASM_REG}, {"BP", TK_ASM_REG}, {"SI", TK_ASM_REG}, {"DI", TK_ASM_REG},
    {"AL", TK_ASM_REG}, {"CL", TK_ASM_REG}, {"DL", TK_ASM_REG}, {"BL", TK_ASM_REG},
    {"AH", TK_ASM_REG}, {"CH", TK_ASM_REG}, {"DH", TK_ASM_REG}, {"BH", TK_ASM_REG},
    
    /* Assembly opcodes */
    {"MOV", TK_ASM_OPCODE}, {"ADD", TK_ASM_OPCODE}, {"SUB", TK_ASM_OPCODE}, {"MUL", TK_ASM_OPCODE},
    {"DIV", TK_ASM_OPCODE}, {"IMUL", TK_ASM_OPCODE}, {"IDIV", TK_ASM_OPCODE}, {"AND", TK_ASM_OPCODE},
    {"OR", TK_ASM_OPCODE}, {"XOR", TK_ASM_OPCODE}, {"NOT", TK_ASM_OPCODE}, {"SHL", TK_ASM_OPCODE},
    {"SHR", TK_ASM_OPCODE}, {"SAR", TK_ASM_OPCODE}, {"ROL", TK_ASM_OPCODE}, {"ROR", TK_ASM_OPCODE},
    {"CMP", TK_ASM_OPCODE}, {"TEST", TK_ASM_OPCODE}, {"JMP", TK_ASM_OPCODE}, {"JE", TK_ASM_OPCODE},
    {"JNE", TK_ASM_OPCODE}, {"JL", TK_ASM_OPCODE}, {"JLE", TK_ASM_OPCODE}, {"JG", TK_ASM_OPCODE},
    {"JGE", TK_ASM_OPCODE}, {"CALL", TK_ASM_OPCODE}, {"RET", TK_ASM_OPCODE}, {"PUSH", TK_ASM_OPCODE},
    {"POP", TK_ASM_OPCODE}, {"LEA", TK_ASM_OPCODE}, {"NOP", TK_ASM_OPCODE},
    
    /* Assembly prefixes */
    {"REP", TK_ASM_PREFIX}, {"REPE", TK_ASM_PREFIX}, {"REPNE", TK_ASM_PREFIX}, {"REPZ", TK_ASM_PREFIX},
    {"REPNZ", TK_ASM_PREFIX}, {"LOCK", TK_ASM_PREFIX},
    
    /* Size specifiers */
    {"BYTE", TK_ASM_SIZE}, {"WORD", TK_ASM_SIZE}, {"DWORD", TK_ASM_SIZE}, {"QWORD", TK_ASM_SIZE},
    
    /* Segment registers */
    {"CS", TK_ASM_SEGMENT}, {"DS", TK_ASM_SEGMENT}, {"ES", TK_ASM_SEGMENT}, {"FS", TK_ASM_SEGMENT},
    {"GS", TK_ASM_SEGMENT}, {"SS", TK_ASM_SEGMENT},
    
    {NULL, TK_EOF}
};

/* Assembly register mapping */
typedef struct {
    const char *name;
    X86Register reg;
} RegMapping;

static RegMapping reg_map[] = {
    {"RAX", REG_RAX}, {"RCX", REG_RCX}, {"RDX", REG_RDX}, {"RBX", REG_RBX},
    {"RSP", REG_RSP}, {"RBP", REG_RBP}, {"RSI", REG_RSI}, {"RDI", REG_RDI},
    {"R8", REG_R8}, {"R9", REG_R9}, {"R10", REG_R10}, {"R11", REG_R11},
    {"R12", REG_R12}, {"R13", REG_R13}, {"R14", REG_R14}, {"R15", REG_R15},
    {"EAX", REG_EAX}, {"ECX", REG_ECX}, {"EDX", REG_EDX}, {"EBX", REG_EBX},
    {"ESP", REG_ESP}, {"EBP", REG_EBP}, {"ESI", REG_ESI}, {"EDI", REG_EDI},
    {"AX", REG_AX}, {"CX", REG_CX}, {"DX", REG_DX}, {"BX", REG_BX},
    {"SP", REG_SP}, {"BP", REG_BP}, {"SI", REG_SI}, {"DI", REG_DI},
    {"AL", REG_AL}, {"CL", REG_CL}, {"DL", REG_DL}, {"BL", REG_BL},
    {"AH", REG_AH}, {"CH", REG_CH}, {"DH", REG_DH}, {"BH", REG_BH},
    {NULL, REG_NONE}
};

/*
 * Lexer management functions
 */

LexerState* lexer_new(FILE *input) {
    LexerState *lexer = (LexerState*)malloc(sizeof(LexerState));
    if (!lexer) return NULL;
    
    /* Initialize all fields */
    memset(lexer, 0, sizeof(LexerState));
    
    lexer->input_file = input;
    lexer->buffer_line = 1;
    lexer->buffer_column = 1;
    lexer->current_token = TK_EOF;
    lexer->token_value = NULL;
    lexer->token_length = 0;
    lexer->token_line = 0;
    lexer->token_column = 0;
    
    /* Initialize assembly state */
    lexer->in_asm_block = false;
    lexer->in_asm_instruction = false;
    lexer->current_reg = REG_NONE;
    
    /* Load file content into buffer */
    if (input) {
        fseek(input, 0, SEEK_END);
        long file_size = ftell(input);
        fseek(input, 0, SEEK_SET);
        
        if (file_size > 0) {
            lexer->input_buffer = (U8*)malloc(file_size + 1);
            if (lexer->input_buffer) {
                size_t bytes_read = fread(lexer->input_buffer, 1, file_size, input);
                lexer->input_buffer[bytes_read] = '\0';
                lexer->buffer_size = bytes_read;
            }
        }
    }
    lexer->current_operand_size = 0;
    
    /* Initialize HolyC state */
    lexer->in_range_expr = false;
    lexer->in_dollar_expr = false;
    lexer->dollar_depth = 0;
    
    /* Initialize error handling */
    lexer->error_count = 0;
    lexer->warning_count = 0;
    lexer->last_error = NULL;
    
    /* Initialize character bitmap */
    lexer->char_bitmap = (U32*)malloc(256 * sizeof(U32));
    if (lexer->char_bitmap) {
        for (I64 i = 0; i < 256; i++) {
            lexer->char_bitmap[i] = 0;
            if (isalpha(i)) lexer->char_bitmap[i] |= 0x01;
            if (isdigit(i)) lexer->char_bitmap[i] |= 0x02;
            if (isalnum(i)) lexer->char_bitmap[i] |= 0x04;
            if (isspace(i)) lexer->char_bitmap[i] |= 0x08;
        }
    }
    
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

/*
 * Character classification functions
 */

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

/*
 * String handling functions
 */

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

/*
 * Error handling functions
 */

void lex_error(LexerState *lexer, const char *message) {
    if (!lexer || !message) return;
    
    lexer->error_count++;
    if (lexer->last_error) free(lexer->last_error);
    lexer->last_error = lex_create_string((U8*)message, strlen(message));
    
    fprintf(stderr, "Lexer Error at line %lld: %s\n", 
            lexer->token_line, message);
}

void lex_warning(LexerState *lexer, const char *message) {
    if (!lexer || !message) return;
    
    lexer->warning_count++;
    fprintf(stderr, "Lexer Warning at line %lld: %s\n", 
            lexer->token_line, message);
}

/*
 * Assembly-specific functions
 */

Bool lex_is_assembly_register(U8 *str) {
    if (!str) return false;
    
    for (I64 i = 0; reg_map[i].name; i++) {
        if (strcmp((const char*)str, reg_map[i].name) == 0) {
            return true;
        }
    }
    return false;
}

X86Register lex_parse_register(U8 *str) {
    if (!str) return REG_NONE;
    
    for (I64 i = 0; reg_map[i].name; i++) {
        if (strcmp((const char*)str, reg_map[i].name) == 0) {
            return reg_map[i].reg;
        }
    }
    return REG_NONE;
}

I64 lex_parse_operand_size(U8 *str) {
    if (!str) return 0;
    
    if (strcmp((const char*)str, "BYTE") == 0) return 1;
    if (strcmp((const char*)str, "WORD") == 0) return 2;
    if (strcmp((const char*)str, "DWORD") == 0) return 4;
    if (strcmp((const char*)str, "QWORD") == 0) return 8;
    
    return 0;
}

Bool lex_is_assembly_opcode(U8 *str) {
    if (!str) return false;
    
    for (I64 i = 0; keywords[i].name; i++) {
        if (keywords[i].token == TK_ASM_OPCODE && 
            strcmp((const char*)str, keywords[i].name) == 0) {
            return true;
        }
    }
    return false;
}

/*
 * HolyC-specific functions
 */

Bool lex_is_holyc_keyword(U8 *str) {
    if (!str) return false;
    
    for (I64 i = 0; keywords[i].name; i++) {
        if (strcmp((const char*)str, keywords[i].name) == 0) {
            return true;
        }
    }
    return false;
}

Bool lex_is_builtin_type(U8 *str) {
    if (!str) return false;
    
    TokenType type = lex_get_builtin_type_token(str);
    return (type != TK_IDENT);
}

TokenType lex_get_builtin_type_token(U8 *str) {
    if (!str) return TK_IDENT;
    
    for (I64 i = 0; keywords[i].name; i++) {
        if (strcmp((const char*)str, keywords[i].name) == 0) {
            TokenType token = keywords[i].token;
            if (token >= TK_TYPE_I0 && token <= TK_TYPE_STRING) {
                return token;
            }
        }
    }
    return TK_IDENT;
}

/*
 * Main tokenization functions
 */

TokenType lex_next_token(LexerState *lexer) {
    if (!lexer) return TK_EOF;
    
    
    /* Skip whitespace */
    while (lexer->buffer_pos < lexer->buffer_size && 
           lex_is_whitespace(lexer->input_buffer[lexer->buffer_pos])) {
        if (lex_is_newline(lexer->input_buffer[lexer->buffer_pos])) {
            lexer->buffer_line++;
            lexer->buffer_column = 1;
        } else {
            lexer->buffer_column++;
        }
        lexer->buffer_pos++;
    }
    
    /* Set token position */
    lexer->token_line = lexer->buffer_line;
    lexer->token_column = lexer->buffer_column;
    
    /* Check for EOF */
    if (lexer->buffer_pos >= lexer->buffer_size) {
        lexer->current_token = TK_EOF;
        return TK_EOF;
    }
    
    U8 c = lexer->input_buffer[lexer->buffer_pos];
    
    /* Handle single character tokens */
    switch (c) {
        case '(': case ')': case '{': case '}': case '[': case ']':
        case ';': case ',': case ':': case '?': case '~':
            lexer->current_token = c;
            lexer->buffer_pos++;
            lexer->buffer_column++;
            return c;
            
        case '+':
            if (lexer->buffer_pos + 1 < lexer->buffer_size &&
                lexer->input_buffer[lexer->buffer_pos + 1] == '+') {
                lexer->current_token = TK_PLUS_PLUS;
                lexer->buffer_pos += 2;
                lexer->buffer_column += 2;
                return TK_PLUS_PLUS;
            } else if (lexer->buffer_pos + 1 < lexer->buffer_size &&
                       lexer->input_buffer[lexer->buffer_pos + 1] == '=') {
                lexer->current_token = TK_ADD_EQU;
                lexer->buffer_pos += 2;
                lexer->buffer_column += 2;
                return TK_ADD_EQU;
            } else {
                lexer->current_token = c;
                lexer->buffer_pos++;
                lexer->buffer_column++;
                return c;
            }
            
        case '-':
            if (lexer->buffer_pos + 1 < lexer->buffer_size &&
                lexer->input_buffer[lexer->buffer_pos + 1] == '-') {
                lexer->current_token = TK_MINUS_MINUS;
                lexer->buffer_pos += 2;
                lexer->buffer_column += 2;
                return TK_MINUS_MINUS;
            } else if (lexer->buffer_pos + 1 < lexer->buffer_size &&
                       lexer->input_buffer[lexer->buffer_pos + 1] == '=') {
                lexer->current_token = TK_SUB_EQU;
                lexer->buffer_pos += 2;
                lexer->buffer_column += 2;
                return TK_SUB_EQU;
            } else if (lexer->buffer_pos + 1 < lexer->buffer_size &&
                       lexer->input_buffer[lexer->buffer_pos + 1] == '>') {
                lexer->current_token = TK_DEREFERENCE;
                lexer->buffer_pos += 2;
                lexer->buffer_column += 2;
                return TK_DEREFERENCE;
            } else {
                lexer->current_token = c;
                lexer->buffer_pos++;
                lexer->buffer_column++;
                return c;
            }
            
        case '*':
            if (lexer->buffer_pos + 1 < lexer->buffer_size &&
                lexer->input_buffer[lexer->buffer_pos + 1] == '=') {
                lexer->current_token = TK_MUL_EQU;
                lexer->buffer_pos += 2;
                lexer->buffer_column += 2;
                return TK_MUL_EQU;
            } else {
                lexer->current_token = c;
                lexer->buffer_pos++;
                lexer->buffer_column++;
                return c;
            }
            
        case '/':
            if (lexer->buffer_pos + 1 < lexer->buffer_size &&
                lexer->input_buffer[lexer->buffer_pos + 1] == '=') {
                lexer->current_token = TK_DIV_EQU;
                lexer->buffer_pos += 2;
                lexer->buffer_column += 2;
                return TK_DIV_EQU;
            } else {
                lexer->current_token = c;
                lexer->buffer_pos++;
                lexer->buffer_column++;
                return c;
            }
            
        case '%':
            if (lexer->buffer_pos + 1 < lexer->buffer_size &&
                lexer->input_buffer[lexer->buffer_pos + 1] == '=') {
                lexer->current_token = TK_MOD_EQU;
                lexer->buffer_pos += 2;
                lexer->buffer_column += 2;
                return TK_MOD_EQU;
            } else {
                lexer->current_token = c;
                lexer->buffer_pos++;
                lexer->buffer_column++;
                return c;
            }
            
        case '=':
            if (lexer->buffer_pos + 1 < lexer->buffer_size &&
                lexer->input_buffer[lexer->buffer_pos + 1] == '=') {
                lexer->current_token = TK_EQU_EQU;
                lexer->buffer_pos += 2;
                lexer->buffer_column += 2;
                return TK_EQU_EQU;
            } else {
                lexer->current_token = c;
                lexer->buffer_pos++;
                lexer->buffer_column++;
                return c;
            }
            
        case '!':
            if (lexer->buffer_pos + 1 < lexer->buffer_size &&
                lexer->input_buffer[lexer->buffer_pos + 1] == '=') {
                lexer->current_token = TK_NOT_EQU;
                lexer->buffer_pos += 2;
                lexer->buffer_column += 2;
                return TK_NOT_EQU;
            } else {
                lexer->current_token = c;
                lexer->buffer_pos++;
                lexer->buffer_column++;
                return c;
            }
            
        case '<':
            if (lexer->buffer_pos + 1 < lexer->buffer_size) {
                U8 next = lexer->input_buffer[lexer->buffer_pos + 1];
                if (next == '=') {
                    lexer->current_token = TK_LESS_EQU;
                    lexer->buffer_pos += 2;
                    lexer->buffer_column += 2;
                    return TK_LESS_EQU;
                } else if (next == '<') {
                    if (lexer->buffer_pos + 2 < lexer->buffer_size &&
                        lexer->input_buffer[lexer->buffer_pos + 2] == '=') {
                        lexer->current_token = TK_SHL_EQU;
                        lexer->buffer_pos += 3;
                        lexer->buffer_column += 3;
                        return TK_SHL_EQU;
                    } else {
                        lexer->current_token = TK_SHL;
                        lexer->buffer_pos += 2;
                        lexer->buffer_column += 2;
                        return TK_SHL;
                    }
                }
            }
            lexer->current_token = c;
            lexer->buffer_pos++;
            lexer->buffer_column++;
            return c;
            
        case '>':
            if (lexer->buffer_pos + 1 < lexer->buffer_size) {
                U8 next = lexer->input_buffer[lexer->buffer_pos + 1];
                if (next == '=') {
                    lexer->current_token = TK_GREATER_EQU;
                    lexer->buffer_pos += 2;
                    lexer->buffer_column += 2;
                    return TK_GREATER_EQU;
                } else if (next == '>') {
                    if (lexer->buffer_pos + 2 < lexer->buffer_size &&
                        lexer->input_buffer[lexer->buffer_pos + 2] == '=') {
                        lexer->current_token = TK_SHR_EQU;
                        lexer->buffer_pos += 3;
                        lexer->buffer_column += 3;
                        return TK_SHR_EQU;
                    } else {
                        lexer->current_token = TK_SHR;
                        lexer->buffer_pos += 2;
                        lexer->buffer_column += 2;
                        return TK_SHR;
                    }
                }
            }
            lexer->current_token = c;
            lexer->buffer_pos++;
            lexer->buffer_column++;
            return c;
            
        case '&':
            if (lexer->buffer_pos + 1 < lexer->buffer_size) {
                U8 next = lexer->input_buffer[lexer->buffer_pos + 1];
                if (next == '=') {
                    lexer->current_token = TK_AND_EQU;
                    lexer->buffer_pos += 2;
                    lexer->buffer_column += 2;
                    return TK_AND_EQU;
                } else if (next == '&') {
                    lexer->current_token = TK_AND_AND;
                    lexer->buffer_pos += 2;
                    lexer->buffer_column += 2;
                    return TK_AND_AND;
                }
            }
            lexer->current_token = c;
            lexer->buffer_pos++;
            lexer->buffer_column++;
            return c;
            
        case '|':
            if (lexer->buffer_pos + 1 < lexer->buffer_size) {
                U8 next = lexer->input_buffer[lexer->buffer_pos + 1];
                if (next == '=') {
                    lexer->current_token = TK_OR_EQU;
                    lexer->buffer_pos += 2;
                    lexer->buffer_column += 2;
                    return TK_OR_EQU;
                } else if (next == '|') {
                    lexer->current_token = TK_OR_OR;
                    lexer->buffer_pos += 2;
                    lexer->buffer_column += 2;
                    return TK_OR_OR;
                }
            }
            lexer->current_token = c;
            lexer->buffer_pos++;
            lexer->buffer_column++;
            return c;
            
        case '^':
            if (lexer->buffer_pos + 1 < lexer->buffer_size) {
                U8 next = lexer->input_buffer[lexer->buffer_pos + 1];
                if (next == '=') {
                    lexer->current_token = TK_XOR_EQU;
                    lexer->buffer_pos += 2;
                    lexer->buffer_column += 2;
                    return TK_XOR_EQU;
                } else if (next == '^') {
                    lexer->current_token = TK_XOR_XOR;
                    lexer->buffer_pos += 2;
                    lexer->buffer_column += 2;
                    return TK_XOR_XOR;
                }
            }
            lexer->current_token = c;
            lexer->buffer_pos++;
            lexer->buffer_column++;
            return c;
            
        case '.':
            if (lexer->buffer_pos + 1 < lexer->buffer_size &&
                lexer->input_buffer[lexer->buffer_pos + 1] == '.') {
                lexer->current_token = TK_DOT_DOT;
                lexer->buffer_pos += 2;
                lexer->buffer_column += 2;
                return TK_DOT_DOT;
            } else {
                lexer->current_token = c;
                lexer->buffer_pos++;
                lexer->buffer_column++;
                return c;
            }
            
        case '"':
            /* String literal */
            return lex_parse_string(lexer);
            
        case '\'':
            /* Character constant */
            return lex_parse_char(lexer);
            
        default:
            if (lex_is_digit(c)) {
                return lex_parse_number(lexer);
            } else if (lex_is_alpha(c) || c == '_') {
                return lex_parse_identifier(lexer);
            } else {
                lexer->current_token = c;
                lexer->buffer_pos++;
                lexer->buffer_column++;
                return c;
            }
    }
}

/* Helper functions for parsing specific token types */


static TokenType lex_parse_string(LexerState *lexer) {
    I64 start_pos = lexer->buffer_pos + 1;  /* Skip opening quote */
    I64 start_col = lexer->buffer_column + 1;
    
    lexer->buffer_pos++;
    lexer->buffer_column++;
    
    while (lexer->buffer_pos < lexer->buffer_size &&
           lexer->input_buffer[lexer->buffer_pos] != '"') {
        if (lexer->input_buffer[lexer->buffer_pos] == '\\') {
            lexer->buffer_pos++;  /* Skip escape character */
            lexer->buffer_column++;
        }
        lexer->buffer_pos++;
        lexer->buffer_column++;
    }
    
    if (lexer->buffer_pos >= lexer->buffer_size) {
        lex_error(lexer, "Unterminated string literal");
        return TK_EOF;
    }
    
    I64 len = lexer->buffer_pos - start_pos;
    lexer->token_value = lex_create_string(&lexer->input_buffer[start_pos], len);
    lexer->token_length = len;
    
    lexer->buffer_pos++;  /* Skip closing quote */
    lexer->buffer_column++;
    
    lexer->current_token = TK_STR;
    return TK_STR;
}

static TokenType lex_parse_char(LexerState *lexer) {
    I64 start_pos = lexer->buffer_pos + 1;  /* Skip opening quote */
    
    lexer->buffer_pos++;
    lexer->buffer_column++;
    
    if (lexer->input_buffer[lexer->buffer_pos] == '\\') {
        lexer->buffer_pos++;  /* Skip escape character */
        lexer->buffer_column++;
    }
    
    lexer->buffer_pos++;
    lexer->buffer_column++;
    
    if (lexer->buffer_pos >= lexer->buffer_size ||
        lexer->input_buffer[lexer->buffer_pos] != '\'') {
        lex_error(lexer, "Unterminated character constant");
        return TK_EOF;
    }
    
    lexer->token_value = lex_create_string(&lexer->input_buffer[start_pos], 1);
    lexer->token_length = 1;
    
    lexer->buffer_pos++;  /* Skip closing quote */
    lexer->buffer_column++;
    
    lexer->current_token = TK_CHAR_CONST;
    return TK_CHAR_CONST;
}

static TokenType lex_parse_number(LexerState *lexer) {
    I64 start_pos = lexer->buffer_pos;
    Bool is_float = false;
    
    while (lexer->buffer_pos < lexer->buffer_size &&
           (lex_is_digit(lexer->input_buffer[lexer->buffer_pos]) ||
            lexer->input_buffer[lexer->buffer_pos] == '.')) {
        if (lexer->input_buffer[lexer->buffer_pos] == '.') {
            is_float = true;
        }
        lexer->buffer_pos++;
        lexer->buffer_column++;
    }
    
    I64 len = lexer->buffer_pos - start_pos;
    lexer->token_value = lex_create_string(&lexer->input_buffer[start_pos], len);
    lexer->token_length = len;
    
    lexer->current_token = is_float ? TK_F64 : TK_I64;
    return lexer->current_token;
}

static TokenType lex_parse_identifier(LexerState *lexer) {
    I64 start_pos = lexer->buffer_pos;
    
    while (lexer->buffer_pos < lexer->buffer_size &&
           (lex_is_alnum(lexer->input_buffer[lexer->buffer_pos]) ||
            lexer->input_buffer[lexer->buffer_pos] == '_')) {
        lexer->buffer_pos++;
        lexer->buffer_column++;
    }
    
    I64 len = lexer->buffer_pos - start_pos;
    lexer->token_value = lex_create_string(&lexer->input_buffer[start_pos], len);
    lexer->token_length = len;
    
    /* Check if it's a keyword */
    for (I64 i = 0; keywords[i].name; i++) {
        if (strcmp((const char*)lexer->token_value, keywords[i].name) == 0) {
            lexer->current_token = keywords[i].token;
            return keywords[i].token;
        }
    }
    
    lexer->current_token = TK_IDENT;
    return TK_IDENT;
}

/* Main tokenization function already implemented above */

/*
 * Utility functions
 */

TokenType lex_peek_token(LexerState *lexer) {
    if (!lexer) return TK_EOF;
    
    I64 saved_pos = lexer->buffer_pos;
    I64 saved_line = lexer->buffer_line;
    I64 saved_column = lexer->buffer_column;
    TokenType saved_token = lexer->current_token;
    U8 *saved_value = lexer->token_value;
    I64 saved_length = lexer->token_length;
    
    TokenType token = lex_next_token(lexer);
    
    lexer->buffer_pos = saved_pos;
    lexer->buffer_line = saved_line;
    lexer->buffer_column = saved_column;
    lexer->current_token = saved_token;
    lexer->token_value = saved_value;
    lexer->token_length = saved_length;
    
    return token;
}

U8* lex_get_token_value(LexerState *lexer) {
    return lexer ? lexer->token_value : NULL;
}

I64 lex_get_token_line(LexerState *lexer) {
    return lexer ? lexer->token_line : 0;
}

I64 lex_get_token_column(LexerState *lexer) {
    return lexer ? lexer->token_column : 0;
}

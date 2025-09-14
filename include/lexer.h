/*
 * Enhanced Lexer for SchismC
 * Ported from TempleOS HolyC with assembly-influenced enhancements
 * 
 * Features:
 * - HolyC-specific syntax support (range comparisons, etc.)
 * - Assembly token recognition
 * - Direct integration with assembly generation
 */

#ifndef LEXER_H
#define LEXER_H

#include "core_structures.h"
#include <stdio.h>

/* Token types - enhanced from original HolyC */
typedef enum {
    /* Basic tokens */
    TK_EOF = 0,
    TK_IDENT = 256,
    
    /* Literals */
    TK_I64,           /* Integer literal */
    TK_F64,           /* Float literal */
    TK_STR,           /* String literal */
    TK_CHAR_CONST,    /* Character constant */
    
    /* HolyC-specific operators */
    TK_DOT_DOT,       /* .. (range operator) */
    TK_ELLIPSIS,      /* ... (ellipsis) */
    TK_SHL,           /* << (shift left) */
    TK_SHR,           /* >> (shift right) */
    TK_SHL_EQU,       /* <<= */
    TK_SHR_EQU,       /* >>= */
    TK_EQU_EQU,       /* == */
    TK_NOT_EQU,       /* != */
    TK_LESS_EQU,      /* <= */
    TK_GREATER_EQU,   /* >= */
    TK_AND_AND,       /* && */
    TK_OR_OR,         /* || */
    TK_XOR_XOR,       /* ^^ */
    TK_PLUS_PLUS,     /* ++ */
    TK_MINUS_MINUS,   /* -- */
    TK_DEREFERENCE,   /* -> (dereference) */
    TK_DBL_COLON,     /* :: (scope resolution) */
    
    /* Assignment operators */
    TK_MUL_EQU,       /* *= */
    TK_DIV_EQU,       /* /= */
    TK_MOD_EQU,       /* %= */
    TK_AND_EQU,       /* &= */
    TK_OR_EQU,        /* |= */
    TK_XOR_EQU,       /* ^= */
    TK_ADD_EQU,       /* += */
    TK_SUB_EQU,       /* -= */
    
    /* Control flow keywords */
    TK_IF,            /* if */
    TK_ELSE,          /* else */
    TK_WHILE,         /* while */
    TK_FOR,           /* for */
    TK_DO,            /* do */
    TK_SWITCH,        /* switch */
    TK_CASE,          /* case */
    TK_DEFAULT,       /* default */
    TK_BREAK,         /* break */
    TK_CONTINUE,      /* continue */
    TK_RETURN,        /* return */
    TK_GOTO,          /* goto */
    
    /* Preprocessor */
    TK_IFDEF,         /* ifdef */
    TK_IFNDEF,        /* ifndef */
    TK_ENDIF,         /* endif */
    TK_DEFINE,        /* define */
    TK_INCLUDE,       /* include */
    
    /* Assembly keywords */
    TK_ASM,           /* asm */
    TK_REG,           /* reg */
    TK_NOREG,         /* noreg */
    TK_INTERRUPT,     /* interrupt */
    TK_HASERRCODE,    /* haserrcode */
    TK_ARGPOP,        /* argpop */
    TK_NOARGPOP,      /* noargpop */
    
    /* Type system */
    TK_CLASS,         /* class */
    TK_UNION,         /* union */
    TK_PUBLIC,        /* public */
    TK_EXTERN,        /* extern */
    TK_IMPORT,        /* import */
    TK_LASTCLASS,     /* lastclass */
    
    /* Exception handling */
    TK_TRY,           /* try */
    TK_CATCH,         /* catch */
    TK_THROW,         /* throw */
    TK_NO_WARN,       /* no_warn */
    
    /* Assembly-specific tokens */
    TK_ASM_REG,       /* Assembly register (RAX, EAX, etc.) */
    TK_ASM_OPCODE,    /* Assembly opcode (MOV, ADD, etc.) */
    TK_ASM_MEMORY,    /* Memory operand [reg+disp] */
    TK_ASM_IMMEDIATE, /* Immediate value */
    TK_ASM_LABEL,     /* Assembly label */
    TK_ASM_SIZE,      /* Size specifier (BYTE, WORD, DWORD, QWORD) */
    TK_ASM_PREFIX,    /* Assembly prefix (REP, LOCK, etc.) */
    TK_ASM_SEGMENT,   /* Segment register (CS, DS, ES, etc.) */
    
    /* x86-64 specific tokens */
    TK_REX_PREFIX,    /* REX prefix */
    TK_MODRM,         /* ModR/M byte */
    TK_SIB,           /* SIB byte */
    TK_DISPLACEMENT,  /* Displacement */
    TK_RELATIVE,      /* RIP-relative addressing */
    
    /* HolyC built-in types */
    TK_TYPE_I0,       /* I0 (zero-size integer) */
    TK_TYPE_I8,       /* I8 (8-bit signed) */
    TK_TYPE_I16,      /* I16 (16-bit signed) */
    TK_TYPE_I32,      /* I32 (32-bit signed) */
    TK_TYPE_I64,      /* I64 (64-bit signed) */
    TK_TYPE_U0,       /* U0 (zero-size unsigned) */
    TK_TYPE_U8,       /* U8 (8-bit unsigned) */
    TK_TYPE_U16,      /* U16 (16-bit unsigned) */
    TK_TYPE_U32,      /* U32 (32-bit unsigned) */
    TK_TYPE_U64,      /* U64 (64-bit unsigned) */
    TK_TYPE_F32,      /* F32 (32-bit float) */
    TK_TYPE_F64,      /* F64 (64-bit float) */
    TK_TYPE_BOOL,     /* Bool (boolean) */
    TK_TYPE_STRING    /* String (char*) */
} TokenType;

/* Lexer state structure */
typedef struct {
    /* Source input */
    FILE *input_file;        /* Input file handle */
    U8 *input_buffer;        /* Input buffer */
    I64 buffer_size;         /* Buffer size */
    I64 buffer_pos;          /* Current position in buffer */
    I64 buffer_line;         /* Current line number */
    I64 buffer_column;       /* Current column number */
    
    /* Current token */
    TokenType current_token; /* Current token type */
    U8 *token_value;         /* Token string value */
    I64 token_length;        /* Token length */
    I64 token_line;          /* Token line number */
    I64 token_column;        /* Token column number */
    
    /* Assembly-specific state */
    Bool in_asm_block;       /* Inside assembly block */
    Bool in_asm_instruction; /* Inside assembly instruction */
    X86Register current_reg; /* Current register being parsed */
    I64 current_operand_size; /* Current operand size */
    
    /* HolyC-specific state */
    Bool in_range_expr;      /* Inside range expression */
    Bool in_dollar_expr;     /* Inside dollar expression */
    I64 dollar_depth;        /* Dollar nesting depth */
    
    /* Error handling */
    I64 error_count;         /* Number of errors */
    I64 warning_count;       /* Number of warnings */
    U8 *last_error;          /* Last error message */
    
    /* Character classification */
    U32 *char_bitmap;        /* Character classification bitmap */
} LexerState;

/* Function prototypes */
LexerState* lexer_new(FILE *input);
void lexer_free(LexerState *lexer);
TokenType lex_next_token(LexerState *lexer);
TokenType lex_peek_token(LexerState *lexer);
U8* lex_get_token_value(LexerState *lexer);
I64 lex_get_token_line(LexerState *lexer);
I64 lex_get_token_column(LexerState *lexer);

/* Assembly-specific functions */
Bool lex_is_assembly_register(U8 *str);
X86Register lex_parse_register(U8 *str);
I64 lex_parse_operand_size(U8 *str);
Bool lex_is_assembly_opcode(U8 *str);

/* HolyC-specific functions */
Bool lex_is_holyc_keyword(U8 *str);
Bool lex_is_builtin_type(U8 *str);
TokenType lex_get_builtin_type_token(U8 *str);

/* Character classification */
Bool lex_is_alpha(U8 c);
Bool lex_is_digit(U8 c);
Bool lex_is_alnum(U8 c);
Bool lex_is_whitespace(U8 c);
Bool lex_is_newline(U8 c);

/* String handling */
U8* lex_create_string(U8 *str, I64 len);
void lex_free_string(U8 *str);

/* Error handling */
void lex_error(LexerState *lexer, const char *message);
void lex_warning(LexerState *lexer, const char *message);

#endif /* LEXER_H */
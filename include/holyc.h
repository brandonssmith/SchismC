/*
 * SchismC - A Windows port of HolyC
 * Core header file defining HolyC types and structures
 */

#ifndef HOLYC_H
#define HOLYC_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* HolyC Built-in Types */
typedef int64_t I64;
typedef double F64;
typedef char* String;
typedef bool Bool;
typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef int8_t I8;
typedef int16_t I16;
typedef int32_t I32;

/* HolyC Built-in Functions */
extern void Print(const char* str, ...);
extern void* MAlloc(I64 size);
extern void Free(void* ptr);
extern String StrNew(const char* str);
extern String StrPrint(const char* fmt, ...);

/* Input Functions */
extern I64 GetI64(const char* prompt, I64 default_val, I64 min_val, I64 max_val);
extern F64 GetF64(const char* prompt, F64 default_val, F64 min_val, F64 max_val);
extern I64 GetString(const char* prompt, char* buffer, I64 buffer_size);
extern void PutChars(const char* str);
extern void PutChar(char c);

/* Type Conversion Functions */
extern I64 ToI64(F64 value);
extern F64 ToF64(I64 value);
extern Bool ToBool(I64 value);

/* File I/O Functions */
extern Bool FileWrite(const char* filename, const void* buf, I64 size, I64 flags);
extern void* FileRead(const char* path, I64* size);

/* Compiler Control Structure - Based on TempleOS CCmpCtrl */
typedef struct {
    I64 pass;                    /* Compilation pass number */
    Bool debug;                  /* Debug mode */
    Bool optimize;               /* Optimization enabled */
    String input_file;           /* Source file being compiled */
    String output_file;          /* Output executable file */
    I64 error_count;             /* Number of compilation errors */
    I64 warning_count;           /* Number of warnings */
} CCmpCtrl;

/* Intermediate Code Structure - Based on TempleOS CIntermediateCode */
typedef struct {
    I64* instructions;           /* Array of instruction codes */
    I64 instruction_count;       /* Number of instructions */
    I64 capacity;                /* Current capacity */
    I64* labels;                 /* Label addresses */
    I64 label_count;             /* Number of labels */
} CIntermediateCode;

/* Assembly Argument Structure - Based on TempleOS CAsmArg */
typedef struct {
    I64 type;                    /* Argument type (register, immediate, memory) */
    I64 value;                   /* Argument value */
    String name;                 /* Symbol name (if applicable) */
    Bool is_label;               /* Is this a label reference? */
} CAsmArg;

/* Ahead-of-Time Compilation Structure - Based on TempleOS CAOT */
typedef struct {
    U8* code;                    /* Generated machine code */
    I64 code_size;               /* Size of generated code */
    I64* imports;                /* Import addresses */
    I64 import_count;            /* Number of imports */
    I64* exports;                /* Export addresses */
    I64 export_count;            /* Number of exports */
    I64 entry_point;             /* Program entry point */
} CAOT;

/* Token Types for Lexer */
typedef enum {
    TK_EOF = 0,
    TK_IDENT,
    TK_NUMBER,
    TK_STRING,
    TK_CHAR,
    TK_I64,
    TK_F64,
    TK_STRING_TYPE,
    TK_BOOL,
    TK_U8, TK_U16, TK_U32, TK_U64,
    TK_I8, TK_I16, TK_I32,
    TK_IF, TK_ELSE, TK_WHILE, TK_FOR,
    TK_RETURN, TK_BREAK, TK_CONTINUE,
    TK_GOTO, TK_ASM, TK_CLASS,
    TK_PLUS, TK_MINUS, TK_MULTIPLY, TK_DIVIDE, TK_MODULO,
    TK_EQUAL, TK_NOT_EQUAL, TK_LESS, TK_GREATER,
    TK_LESS_EQUAL, TK_GREATER_EQUAL,
    TK_AND, TK_OR, TK_NOT,
    TK_ASSIGN, TK_SEMICOLON, TK_COMMA,
    TK_LPAREN, TK_RPAREN, TK_LBRACE, TK_RBRACE,
    TK_LBRACKET, TK_RBRACKET,
    TK_DOT, TK_ARROW,
    TK_COLON, TK_QUESTION,
    TK_PLUS_PLUS, TK_MINUS_MINUS,
    TK_PLUS_EQUAL, TK_MINUS_EQUAL,
    TK_MULTIPLY_EQUAL, TK_DIVIDE_EQUAL,
    TK_MODULO_EQUAL,
    TK_AND_EQUAL, TK_OR_EQUAL, TK_XOR_EQUAL,
    TK_LEFT_SHIFT, TK_RIGHT_SHIFT,
    TK_LEFT_SHIFT_EQUAL, TK_RIGHT_SHIFT_EQUAL,
    TK_AND_AND, TK_OR_OR,
    TK_EQUAL_EQUAL,
    TK_DEREFERENCE,
    TK_RANGE,                    /* HolyC range operator (..) */
    TK_COUNT
} TokenType;

/* Token Structure */
typedef struct {
    TokenType type;
    String value;
    I64 line;
    I64 column;
    I64 number_value;            /* For numeric tokens */
    F64 float_value;             /* For float tokens */
} Token;

/* AST Node Types */
typedef enum {
    NODE_PROGRAM,
    NODE_FUNCTION,
    NODE_VARIABLE,
    NODE_ASSIGNMENT,
    NODE_BINARY_OP,
    NODE_UNARY_OP,
    NODE_CALL,
    NODE_IF,
    NODE_WHILE,
    NODE_FOR,
    NODE_RETURN,
    NODE_BREAK,
    NODE_CONTINUE,
    NODE_GOTO,
    NODE_ASM,
    NODE_CLASS,
    NODE_IDENTIFIER,
    NODE_NUMBER,
    NODE_STRING,
    NODE_CHAR_CONST,
    NODE_ARRAY_ACCESS,
    NODE_MEMBER_ACCESS,
    NODE_BLOCK,
    NODE_COUNT
} ASTNodeType;

/* AST Node Structure */
typedef struct ASTNode {
    ASTNodeType type;
    String name;                 /* Function/variable name */
    String value;                /* String/numeric value */
    I64 number_value;            /* Numeric value */
    F64 float_value;             /* Float value */
    I64 line;                    /* Source line */
    I64 column;                  /* Source column */
    struct ASTNode* children;    /* First child */
    struct ASTNode* next;        /* Next sibling */
    struct ASTNode* parent;      /* Parent node */
} ASTNode;

/* Compiler Context - Global state */
typedef struct {
    CCmpCtrl ctrl;
    Token* tokens;
    I64 token_count;
    I64 current_token;
    ASTNode* ast_root;
    CIntermediateCode ic;
    CAOT aot;
    String output_code;
    I64 error_count;
    I64 warning_count;
} CompilerContext;

/* Global compiler context */
extern CompilerContext g_ctx;

/* Function prototypes */
extern void init_compiler(void);
extern void cleanup_compiler(void);
extern Bool compile_file(const char* input_file, const char* output_file);

#endif /* HOLYC_H */
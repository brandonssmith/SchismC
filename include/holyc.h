#ifndef HOLYC_H
#define HOLYC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

// HolyC built-in types
typedef void U0;  // void (zero size)
typedef int8_t I8;
typedef uint8_t U8;
typedef int16_t I16;
typedef uint16_t U16;
typedef int32_t I32;
typedef uint32_t U32;
typedef int64_t I64;
typedef uint64_t U64;
typedef double F64;
typedef bool Bool;

// String type (char*)
typedef char* String;

// Constants
#define TRUE 1
#define FALSE 0

// Token types
typedef enum {
    TK_EOF = 0,
    TK_IDENT = 256,
    TK_I64,
    TK_F64,
    TK_STR,
    TK_CHAR_CONST,
    TK_DOT_DOT,
    TK_ELLIPSIS,
    TK_SHL,
    TK_SHR,
    TK_SHL_EQU,
    TK_SHR_EQU,
    TK_EQU_EQU,
    TK_NOT_EQU,
    TK_LESS_EQU,
    TK_GREATER_EQU,
    TK_AND_AND,
    TK_OR_OR,
    TK_XOR_XOR,
    TK_PLUS_PLUS,
    TK_MINUS_MINUS,
    TK_DEREFERENCE,
    TK_DBL_COLON,
    TK_MUL_EQU,
    TK_DIV_EQU,
    TK_MOD_EQU,
    TK_AND_EQU,
    TK_OR_EQU,
    TK_XOR_EQU,
    TK_ADD_EQU,
    TK_SUB_EQU,
    TK_IF,
    TK_ELSE,
    TK_WHILE,
    TK_FOR,
    TK_DO,
    TK_SWITCH,
    TK_CASE,
    TK_DEFAULT,
    TK_BREAK,
    TK_CONTINUE,
    TK_RETURN,
    TK_GOTO,
    TK_IFDEF,
    TK_IFNDEF,
    TK_ENDIF,
    TK_DEFINE,
    TK_INCLUDE,
    TK_ASM,
    TK_CLASS,
    TK_UNION,
    TK_PUBLIC,
    TK_EXTERN,
    TK_IMPORT,
    TK_TRY,
    TK_CATCH,
    TK_THROW,
    TK_NO_WARN,
    TK_REG,
    TK_NOREG,
    TK_INTERRUPT,
    TK_HASERRCODE,
    TK_ARGPOP,
    TK_NOARGPOP,
    TK_LASTCLASS
} TokenType;

// Node types for AST
typedef enum {
    NODE_PROGRAM,
    NODE_FUNCTION,
    NODE_CLASS,
    NODE_VARIABLE,
    NODE_ASSIGNMENT,
    NODE_BINARY_OP,
    NODE_UNARY_OP,
    NODE_CALL,
    NODE_IF,
    NODE_WHILE,
    NODE_FOR,
    NODE_SWITCH,
    NODE_CASE,
    NODE_RETURN,
    NODE_BREAK,
    NODE_CONTINUE,
    NODE_GOTO,
    NODE_LABEL,
    NODE_ASM,
    NODE_STRING,
    NODE_NUMBER,
    NODE_IDENTIFIER,
    NODE_CHAR_CONST,
    NODE_ARRAY_ACCESS,
    NODE_MEMBER_ACCESS,
    NODE_CAST,
    NODE_TERNARY,
    NODE_RANGE_CMP
} NodeType;

// Binary operators
typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_SHL,
    OP_SHR,
    OP_AND,
    OP_OR,
    OP_XOR,
    OP_EQU,
    OP_NE,
    OP_LT,
    OP_LE,
    OP_GT,
    OP_GE,
    OP_AND_AND,
    OP_OR_OR,
    OP_XOR_XOR,
    OP_ASSIGN,
    OP_ADD_ASSIGN,
    OP_SUB_ASSIGN,
    OP_MUL_ASSIGN,
    OP_DIV_ASSIGN,
    OP_MOD_ASSIGN,
    OP_AND_ASSIGN,
    OP_OR_ASSIGN,
    OP_XOR_ASSIGN,
    OP_SHL_ASSIGN,
    OP_SHR_ASSIGN,
    OP_POWER
} BinaryOp;

// Unary operators
typedef enum {
    OP_PRE_INC,
    OP_PRE_DEC,
    OP_POST_INC,
    OP_POST_DEC,
    OP_PLUS,
    OP_MINUS,
    OP_NOT,
    OP_COMP,
    OP_DEREF,
    OP_ADDR,
    OP_SIZEOF
} UnaryOp;

// AST Node structure
typedef struct ASTNode {
    NodeType type;
    union {
        I64 int_val;
        F64 float_val;
        String str_val;
        char char_val;
        BinaryOp binary_op;
        UnaryOp unary_op;
    } value;
    struct ASTNode* left;
    struct ASTNode* right;
    struct ASTNode* children;  // For function bodies, class members, etc.
    struct ASTNode* next;      // For linked lists
    String name;               // For identifiers, function names, etc.
    I64 line_num;
    I64 col_num;
} ASTNode;

// Symbol table entry
typedef struct Symbol {
    String name;
    I64 type;  // Token type for built-in types
    I64 size;
    I64 offset;
    Bool is_function;
    Bool is_global;
    struct Symbol* next;
} Symbol;

// Compiler context
typedef struct CompilerContext {
    FILE* input_file;
    String filename;
    I64 line_num;
    I64 col_num;
    TokenType current_token;
    String current_string;
    I64 current_int;
    F64 current_float;
    char current_char;
    Symbol* symbol_table;
    ASTNode* ast_root;
    String output_code;
    I64 error_count;
    I64 warning_count;
} CompilerContext;

// Function prototypes
// Lexer
TokenType lex_next_token(CompilerContext* ctx);
void lex_init(CompilerContext* ctx, FILE* file, const char* filename);
void lex_cleanup(CompilerContext* ctx);

// Parser
ASTNode* parse_program(CompilerContext* ctx);
ASTNode* parse_statement(CompilerContext* ctx);
ASTNode* parse_expression(CompilerContext* ctx);
ASTNode* parse_function(CompilerContext* ctx);
ASTNode* parse_class(CompilerContext* ctx);
ASTNode* parse_if_statement(CompilerContext* ctx);
ASTNode* parse_while_statement(CompilerContext* ctx);
ASTNode* parse_for_statement(CompilerContext* ctx);
ASTNode* parse_return_statement(CompilerContext* ctx);
ASTNode* parse_break_statement(CompilerContext* ctx);
ASTNode* parse_continue_statement(CompilerContext* ctx);
ASTNode* parse_goto_statement(CompilerContext* ctx);
ASTNode* parse_asm_statement(CompilerContext* ctx);
ASTNode* parse_class_definition(CompilerContext* ctx);
ASTNode* parse_print_statement(CompilerContext* ctx);
ASTNode* parse_expression_statement(CompilerContext* ctx);
ASTNode* parse_assignment_expression(CompilerContext* ctx);
ASTNode* parse_logical_or_expression(CompilerContext* ctx);
ASTNode* parse_logical_and_expression(CompilerContext* ctx);
ASTNode* parse_equality_expression(CompilerContext* ctx);
ASTNode* parse_relational_expression(CompilerContext* ctx);
ASTNode* parse_shift_expression(CompilerContext* ctx);
ASTNode* parse_additive_expression(CompilerContext* ctx);
ASTNode* parse_multiplicative_expression(CompilerContext* ctx);
ASTNode* parse_unary_expression(CompilerContext* ctx);
ASTNode* parse_postfix_expression(CompilerContext* ctx);
ASTNode* parse_primary_expression(CompilerContext* ctx);
void parser_cleanup(ASTNode* node);

// Code generator
String generate_code(CompilerContext* ctx, ASTNode* node);
String generate_program(CompilerContext* ctx, ASTNode* node);
String generate_function(CompilerContext* ctx, ASTNode* node);
String generate_class(CompilerContext* ctx, ASTNode* node);
String generate_variable(CompilerContext* ctx, ASTNode* node);
String generate_assignment(CompilerContext* ctx, ASTNode* node);
String generate_binary_op(CompilerContext* ctx, ASTNode* node);
String generate_unary_op(CompilerContext* ctx, ASTNode* node);
String generate_call(CompilerContext* ctx, ASTNode* node);
String generate_if(CompilerContext* ctx, ASTNode* node);
String generate_while(CompilerContext* ctx, ASTNode* node);
String generate_for(CompilerContext* ctx, ASTNode* node);
String generate_return(CompilerContext* ctx, ASTNode* node);
String generate_break(CompilerContext* ctx, ASTNode* node);
String generate_continue(CompilerContext* ctx, ASTNode* node);
String generate_goto(CompilerContext* ctx, ASTNode* node);
String generate_label(CompilerContext* ctx, ASTNode* node);
String generate_asm(CompilerContext* ctx, ASTNode* node);
String generate_string(CompilerContext* ctx, ASTNode* node);
String generate_number(CompilerContext* ctx, ASTNode* node);
String generate_identifier(CompilerContext* ctx, ASTNode* node);
String generate_char_const(CompilerContext* ctx, ASTNode* node);
String generate_array_access(CompilerContext* ctx, ASTNode* node);
String generate_member_access(CompilerContext* ctx, ASTNode* node);
String generate_cast(CompilerContext* ctx, ASTNode* node);
String generate_ternary(CompilerContext* ctx, ASTNode* node);
String generate_range_cmp(CompilerContext* ctx, ASTNode* node);

// Runtime functions
void Print(const char* format, ...);
void PutChars(const char* str);
I64 StrLen(const char* str);
String StrNew(const char* str);
String StrPrint(const char* format, ...);
String StrPrintJoin(String existing, const char* format, ...);
void Free(void* ptr);
void* MAlloc(I64 size);

// Utility functions
void error(CompilerContext* ctx, const char* message, ...);
void warning(CompilerContext* ctx, const char* message, ...);
ASTNode* create_node(NodeType type);
void free_node(ASTNode* node);
Symbol* add_symbol(CompilerContext* ctx, const char* name, I64 type, I64 size);
Symbol* find_symbol(CompilerContext* ctx, const char* name);

#endif // HOLYC_H

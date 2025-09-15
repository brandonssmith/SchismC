/*
 * Enhanced Parser for SchismC
 * Ported from TempleOS HolyC with assembly-influenced enhancements
 * 
 * Features:
 * - Statement parsing (PrsStmt.HC equivalent)
 * - Expression parsing (PrsExp.HC equivalent)
 * - Variable parsing (PrsVar.HC equivalent)
 * - Assembly parsing (PrsAsm* functions equivalent)
 * - Direct integration with assembly generation
 */

#ifndef PARSER_H
#define PARSER_H

#include "core_structures.h"
#include "lexer.h"

/* AST Node types - enhanced for assembly generation */
typedef enum {
    NODE_PROGRAM = 1,
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
    NODE_BLOCK,
    NODE_IDENTIFIER,
    NODE_INTEGER,
    NODE_FLOAT,
    NODE_STRING,
    NODE_CHAR,
    NODE_BOOLEAN,
    
    /* Assembly-specific nodes */
    NODE_ASM_BLOCK,
    NODE_ASM_INSTRUCTION,
    NODE_ASM_OPERAND,
    NODE_ASM_REGISTER,
    NODE_ASM_MEMORY,
    NODE_ASM_IMMEDIATE,
    NODE_ASM_LABEL,
    NODE_ASM_DIRECTIVE,
    
    /* HolyC-specific nodes */
    NODE_RANGE_EXPR,
    NODE_DOLLAR_EXPR,
    NODE_CLASS_DEF,
    NODE_UNION_DEF,
    NODE_PUBLIC_DECL,
    NODE_EXTERN_DECL,
    NODE_IMPORT_DECL,
    NODE_TRY_CATCH,
    NODE_THROW,
    
    /* Type system nodes */
    NODE_TYPE_DECL,
    NODE_TYPE_SPECIFIER,
    NODE_TYPE_CAST,
    NODE_TYPE_REF,
    NODE_TYPE_DEREF,
    NODE_ARRAY_ACCESS,
    NODE_MEMBER_ACCESS,
    NODE_POINTER_ARITH,
    
    /* Control flow nodes */
    NODE_IF_STMT,
    NODE_WHILE_STMT,
    NODE_FOR_STMT,
    NODE_SWITCH,
    NODE_CASE,
    NODE_DEFAULT,
    NODE_BREAK,
    NODE_CONTINUE,
    NODE_GOTO,
    NODE_LABEL
} ASTNodeType;

/* Binary operator types */
typedef enum {
    BINOP_ADD,        /* + */
    BINOP_SUB,        /* - */
    BINOP_MUL,        /* * */
    BINOP_DIV,        /* / */
    BINOP_MOD,        /* % */
    BINOP_AND,        /* & */
    BINOP_OR,         /* | */
    BINOP_XOR,        /* ^ */
    BINOP_SHL,        /* << */
    BINOP_SHR,        /* >> */
    BINOP_EQ,         /* == */
    BINOP_NE,         /* != */
    BINOP_LT,         /* < */
    BINOP_LE,         /* <= */
    BINOP_GT,         /* > */
    BINOP_GE,         /* >= */
    BINOP_AND_AND,    /* && */
    BINOP_OR_OR,      /* || */
    BINOP_XOR_XOR,    /* ^^ */
    BINOP_ASSIGN,     /* = */
    BINOP_ADD_ASSIGN, /* += */
    BINOP_SUB_ASSIGN, /* -= */
    BINOP_MUL_ASSIGN, /* *= */
    BINOP_DIV_ASSIGN, /* /= */
    BINOP_MOD_ASSIGN, /* %= */
    BINOP_AND_ASSIGN, /* &= */
    BINOP_OR_ASSIGN,  /* |= */
    BINOP_XOR_ASSIGN, /* ^= */
    BINOP_SHL_ASSIGN, /* <<= */
    BINOP_SHR_ASSIGN, /* >>= */
    
    /* HolyC-specific operators */
    BINOP_RANGE,      /* .. */
    BINOP_DOT_DOT     /* .. */
} BinaryOpType;

/* Unary operator types */
typedef enum {
    UNOP_PLUS,        /* + */
    UNOP_MINUS,       /* - */
    UNOP_NOT,         /* ! */
    UNOP_BITNOT,      /* ~ */
    UNOP_INC,         /* ++ */
    UNOP_DEC,         /* -- */
    UNOP_DEREF,       /* * */
    UNOP_ADDR,        /* & */
    UNOP_DEREFERENCE  /* -> */
} UnaryOpType;

/* AST Node structure - assembly-aware */
typedef struct ASTNode {
    ASTNodeType type;
    I64 line;                 /* Source line number */
    I64 column;               /* Source column number */
    
    /* Node data */
    union {
        /* Program structure */
        struct {
            struct ASTNode *functions;
            struct ASTNode *globals;
            struct ASTNode *imports;
        } program;
        
        /* Function definition */
        struct {
            U8 *name;         /* Function name */
            U8 *return_type;  /* Return type */
            struct ASTNode *parameters;  /* Parameter list */
            struct ASTNode *body;        /* Function body */
            Bool is_public;   /* Public function */
            Bool is_extern;   /* External function */
            Bool is_reg;      /* Register function */
            Bool is_interrupt; /* Interrupt function */
            I64 stack_size;   /* Stack frame size */
            I64 register_count; /* Number of registers used */
        } function;
        
        /* Variable declaration */
        struct {
            U8 *name;         /* Variable name */
            U8 *type;         /* Variable type */
            struct ASTNode *initializer; /* Initial value */
            Bool is_public;   /* Public variable */
            Bool is_extern;   /* External variable */
            Bool is_global;   /* Global variable */
            Bool is_parameter; /* Function parameter */
            I64 parameter_index; /* Parameter index (0-based) */
            I64 stack_offset; /* Stack offset for locals */
            I64 global_offset; /* Global data offset */
            I64 size;         /* Variable size in bytes */
        } variable;
        
        /* Binary operation */
        struct {
            BinaryOpType op;  /* Operation type */
            struct ASTNode *left;   /* Left operand */
            struct ASTNode *right;  /* Right operand */
            U8 *result_type;  /* Result type */
            X86Register result_reg; /* Result register */
            Bool is_assembly; /* Generates assembly directly */
        } binary_op;
        
        /* Unary operation */
        struct {
            UnaryOpType op;   /* Operation type */
            struct ASTNode *operand; /* Operand */
            U8 *result_type;  /* Result type */
            X86Register result_reg; /* Result register */
            Bool is_assembly; /* Generates assembly directly */
        } unary_op;
        
        /* Function call */
        struct {
            U8 *name;         /* Function name */
            struct ASTNode *arguments; /* Argument list */
            U8 *return_type;  /* Return type */
            X86Register return_reg; /* Return register */
            I64 arg_count;    /* Number of arguments */
            I64 stack_cleanup; /* Stack cleanup size */
        } call;
        
        /* Control flow */
        struct {
            struct ASTNode *condition; /* Condition expression */
            struct ASTNode *then_body; /* Then branch */
            struct ASTNode *else_body; /* Else branch */
            struct ASTNode *cases;     /* Switch cases */
            struct ASTNode *default_case; /* Default case */
            U8 *label_name;   /* Label name for goto */
            I64 jump_target;  /* Jump target address */
        } control;
        
        /* Block statement */
        struct {
            struct ASTNode *statements; /* List of statements */
            I64 statement_count; /* Number of statements */
            struct ASTNode *local_vars; /* Local variables in this block */
            I64 local_var_count; /* Number of local variables */
        } block;
        
        /* Assembly block */
        struct {
            struct ASTNode *instructions; /* Assembly instructions */
            Bool is_inline;   /* Inline assembly */
            Bool is_volatile; /* Volatile assembly */
            I64 input_count;  /* Number of input operands */
            I64 output_count; /* Number of output operands */
            I64 clobber_count; /* Number of clobbered registers */
            U8 **input_ops;   /* Input operands */
            U8 **output_ops;  /* Output operands */
            U8 **clobber_ops; /* Clobbered registers */
        } asm_block;
        
        /* Assembly instruction */
        struct {
            U8 *opcode;       /* Assembly opcode */
            struct ASTNode *operands; /* Operand list */
            I64 operand_count; /* Number of operands */
            I64 instruction_size; /* Instruction size in bytes */
            U8 *encoding;     /* Encoded instruction bytes */
            I64 encoding_size; /* Size of encoded instruction */
        } asm_instruction;
        
        /* Assembly operand */
        struct {
            CAsmArg *arg;     /* Assembly argument */
            Bool is_input;    /* Input operand */
            Bool is_output;   /* Output operand */
            Bool is_clobber;  /* Clobbered register */
            U8 *constraint;   /* Constraint string */
        } asm_operand;
        
        /* Literal values */
        struct {
            I64 i64_value;    /* Integer value */
            F64 f64_value;    /* Float value */
            U8 *str_value;    /* String value */
            Bool bool_value;  /* Boolean value */
            U8 char_value;    /* Character value */
            U8 *type_name;    /* Type name */
            I64 size;         /* Size in bytes */
        } literal;
        
        /* Identifier */
        struct {
            U8 *name;         /* Identifier name */
            U8 *type;         /* Identifier type */
            struct ASTNode *declaration; /* Variable/function declaration */
            X86Register register_id; /* Allocated register */
            I64 stack_offset; /* Stack offset */
            Bool is_global;   /* Global identifier */
            Bool is_parameter; /* Function parameter */
        } identifier;
        
        /* Range expression (HolyC specific) */
        struct {
            struct ASTNode *start; /* Start value */
            struct ASTNode *end;   /* End value */
            struct ASTNode *step;  /* Step value (optional) */
            Bool is_inclusive;     /* Inclusive range */
        } range_expr;
        
        /* Dollar expression (HolyC specific) */
        struct {
            struct ASTNode *expression; /* Expression to evaluate */
            I64 depth;         /* Dollar nesting depth */
            Bool is_string;    /* String interpolation */
        } dollar_expr;
        
        /* Type declaration */
        struct {
            U8 *name;         /* Type name */
            U8 *base_type;    /* Base type */
            I64 size;         /* Type size */
            I64 alignment;    /* Type alignment */
            struct ASTNode *members; /* Type members */
            Bool is_struct;   /* Structure type */
            Bool is_union;    /* Union type */
            Bool is_enum;     /* Enumeration type */
        } type_decl;
        
        /* Type specifier */
        struct {
            TokenType type;   /* Token type (TK_TYPE_I64, etc.) */
            U8 *type_name;    /* Type name string */
            I64 size;         /* Type size in bytes */
            Bool is_signed;   /* Signed type */
            Bool is_floating; /* Floating point type */
        } type_specifier;
        
        /* Assignment */
        struct {
            struct ASTNode *left;   /* Left-hand side (variable) */
            struct ASTNode *right;  /* Right-hand side (expression) */
            BinaryOpType op;        /* Assignment operator */
        } assignment;
        
        /* Return statement */
        struct {
            struct ASTNode *expression; /* Return expression (optional) */
            I64 return_value;      /* Return value (for simple cases) */
            U8 *return_type;       /* Return type */
            X86Register return_reg; /* Return register */
        } return_stmt;
        
        /* If statement */
        struct {
            struct ASTNode *condition;  /* Condition expression */
            struct ASTNode *then_stmt;  /* Then statement */
            struct ASTNode *else_stmt;  /* Else statement (optional) */
        } if_stmt;
        
        /* While statement */
        struct {
            struct ASTNode *condition;  /* Loop condition */
            struct ASTNode *body_stmt;  /* Loop body statement */
        } while_stmt;
        
        /* Boolean literal */
        struct {
            Bool value;        /* Boolean value */
        } boolean;
    } data;
    
    /* AST navigation */
    struct ASTNode *next;     /* Next sibling */
    struct ASTNode *prev;     /* Previous sibling */
    struct ASTNode *parent;   /* Parent node */
    struct ASTNode *children; /* First child */
    
    /* Assembly generation state */
    Bool assembly_generated;  /* Assembly code generated */
    U8 *assembly_code;        /* Generated assembly code */
    I64 assembly_size;        /* Assembly code size */
    CIntermediateCode *intermediate; /* Intermediate code */
    
} ASTNode;

/* Scope level structure for variable scope management */
typedef struct ScopeLevel {
    struct ScopeLevel *parent;      /* Parent scope */
    ASTNode **variables;            /* Variables in this scope */
    I64 variable_count;             /* Number of variables in scope */
    I64 variable_capacity;          /* Capacity of variables array */
    I64 scope_id;                   /* Unique scope identifier */
    I64 stack_offset;               /* Stack offset for local variables */
    Bool is_function_scope;         /* Whether this is a function scope */
    Bool is_block_scope;            /* Whether this is a block scope */
} ScopeLevel;

/* Parser state structure */
typedef struct {
    LexerState *lexer;        /* Lexer state */
    CCmpCtrl *cc;             /* Compiler control */
    ASTNode *root;            /* Root AST node */
    ASTNode *current_node;    /* Current parsing node */
    
    /* Parsing state */
    I64 error_count;          /* Number of parsing errors */
    I64 warning_count;        /* Number of parsing warnings */
    U8 *last_error;           /* Last error message */
    
    /* Position tracking for lookahead */
    I64 saved_buffer_pos;     /* Saved buffer position */
    I64 saved_buffer_line;    /* Saved buffer line */
    I64 saved_buffer_column;  /* Saved buffer column */
    TokenType saved_current_token; /* Saved current token */
    U8 *saved_token_value;    /* Saved token value */
    I64 saved_token_length;   /* Saved token length */
    Bool position_saved;      /* Whether position is saved */
    
    /* Assembly parsing state */
    Bool in_asm_block;        /* Inside assembly block */
    Bool in_asm_instruction;  /* Inside assembly instruction */
    X86Register current_reg;  /* Current register context */
    I64 current_operand_size; /* Current operand size */
    
    /* HolyC parsing state */
    Bool in_range_expr;       /* Inside range expression */
    Bool in_dollar_expr;      /* Inside dollar expression */
    I64 dollar_depth;         /* Dollar nesting depth */
    I64 class_depth;          /* Class nesting depth */
    
    /* Symbol table */
    struct {
        ASTNode **symbols;    /* Symbol table */
        I64 count;            /* Number of symbols */
        I64 capacity;         /* Table capacity */
        
        /* Address tracking */
        I64 current_address;  /* Current address for next symbol */
        I64 function_offset;  /* Offset for function addresses */
        I64 variable_offset;  /* Offset for variable addresses */
    } symbol_table;
    
    /* Scope management */
    struct {
        struct ScopeLevel **scopes;  /* Stack of scope levels */
        I64 scope_count;             /* Number of active scopes */
        I64 scope_capacity;          /* Scope stack capacity */
        I64 current_scope_depth;     /* Current scope depth */
    } scope_stack;
    
} ParserState;

/* Function prototypes */

/* Parser management */
ParserState* parser_new(LexerState *lexer, CCmpCtrl *cc);
void parser_free(ParserState *parser);
ASTNode* parse_program(ParserState *parser);

/* Statement parsing */
ASTNode* parse_statement(ParserState *parser);
ASTNode* parse_expression_statement(ParserState *parser);
ASTNode* parse_if_statement(ParserState *parser);
ASTNode* parse_while_statement(ParserState *parser);
ASTNode* parse_for_statement(ParserState *parser);
ASTNode* parse_switch_statement(ParserState *parser);
ASTNode* parse_return_statement(ParserState *parser);
ASTNode* parse_break_statement(ParserState *parser);
ASTNode* parse_continue_statement(ParserState *parser);
ASTNode* parse_goto_statement(ParserState *parser);
ASTNode* parse_label_statement(ParserState *parser);
ASTNode* parse_block_statement(ParserState *parser);

/* Expression parsing */
ASTNode* parse_expression(ParserState *parser);
ASTNode* parse_assignment_expression(ParserState *parser);
ASTNode* parse_conditional_expression(ParserState *parser);
ASTNode* parse_logical_or_expression(ParserState *parser);
ASTNode* parse_logical_and_expression(ParserState *parser);
ASTNode* parse_bitwise_or_expression(ParserState *parser);
ASTNode* parse_bitwise_xor_expression(ParserState *parser);
ASTNode* parse_bitwise_and_expression(ParserState *parser);
ASTNode* parse_equality_expression(ParserState *parser);
ASTNode* parse_relational_expression(ParserState *parser);
ASTNode* parse_shift_expression(ParserState *parser);
ASTNode* parse_additive_expression(ParserState *parser);
ASTNode* parse_multiplicative_expression(ParserState *parser);
ASTNode* parse_unary_expression(ParserState *parser);
ASTNode* parse_postfix_expression(ParserState *parser);
ASTNode* parse_primary_expression(ParserState *parser);

/* Variable and function parsing */
ASTNode* parse_variable_declaration(ParserState *parser);
ASTNode* parse_function_declaration(ParserState *parser);
ASTNode* parse_function_call(ParserState *parser, U8 *name, I64 line, I64 column);
ASTNode* parse_parameter_list(ParserState *parser);
ASTNode* parse_argument_list(ParserState *parser);
ASTNode* parse_type_specifier(ParserState *parser);

/* Assembly parsing */
ASTNode* parse_assembly_block(ParserState *parser);
ASTNode* parse_assembly_instruction(ParserState *parser);
ASTNode* parse_assembly_operand(ParserState *parser);
ASTNode* parse_assembly_register(ParserState *parser);
ASTNode* parse_assembly_memory(ParserState *parser);
ASTNode* parse_assembly_immediate(ParserState *parser);
ASTNode* parse_assembly_label(ParserState *parser);

/* HolyC-specific parsing */
ASTNode* parse_range_expression(ParserState *parser);
ASTNode* parse_dollar_expression(ParserState *parser);
ASTNode* parse_class_definition(ParserState *parser);
ASTNode* parse_union_definition(ParserState *parser);
ASTNode* parse_public_declaration(ParserState *parser);
ASTNode* parse_extern_declaration(ParserState *parser);
ASTNode* parse_import_declaration(ParserState *parser);
ASTNode* parse_try_catch_block(ParserState *parser);
ASTNode* parse_throw_statement(ParserState *parser);

/* Type system parsing */
ASTNode* parse_type_declaration(ParserState *parser);
ASTNode* parse_type_cast(ParserState *parser);
ASTNode* parse_type_reference(ParserState *parser);
ASTNode* parse_type_dereference(ParserState *parser);
ASTNode* parse_array_access(ParserState *parser);
ASTNode* parse_member_access(ParserState *parser);
ASTNode* parse_pointer_arithmetic(ParserState *parser);

/* AST node management */
ASTNode* ast_node_new(ASTNodeType type, I64 line, I64 column);
void ast_node_free(ASTNode *node);
void ast_node_add_child(ASTNode *parent, ASTNode *child);
void ast_node_add_sibling(ASTNode *node, ASTNode *sibling);

/* Utility functions */
TokenType parser_expect_token(ParserState *parser, TokenType expected);
Bool parser_match_token(ParserState *parser, TokenType token);
TokenType parser_current_token(ParserState *parser);
TokenType parser_next_token(ParserState *parser);
U8* parser_current_token_value(ParserState *parser);
I64 parser_current_line(ParserState *parser);
I64 parser_current_column(ParserState *parser);

/* Parser position management */
void parser_save_position(ParserState *parser);
void parser_restore_position(ParserState *parser);

/* Scope management */
ScopeLevel* scope_level_new(ParserState *parser, Bool is_function_scope, Bool is_block_scope);
void scope_level_free(ScopeLevel *scope);
Bool parser_enter_scope(ParserState *parser, Bool is_function_scope, Bool is_block_scope);
Bool parser_exit_scope(ParserState *parser);
ScopeLevel* parser_get_current_scope(ParserState *parser);
Bool scope_add_variable(ScopeLevel *scope, ASTNode *variable);
ASTNode* scope_lookup_variable(ScopeLevel *scope, U8 *name);
ASTNode* parser_lookup_variable_in_scope(ParserState *parser, U8 *name);
Bool parser_is_variable_defined_in_scope(ParserState *parser, U8 *name);

/* Error handling */
void parser_error(ParserState *parser, U8 *message);
void parser_warning(ParserState *parser, U8 *message);
void parser_expected_error(ParserState *parser, TokenType expected, TokenType found);

/* Symbol table management */
void parser_add_symbol(ParserState *parser, U8 *name, ASTNode *declaration);
ASTNode* parser_lookup_symbol(ParserState *parser, U8 *name);
Bool parser_is_symbol_defined(ParserState *parser, U8 *name);

/* Address calculation */
I64 parser_calculate_function_address(ParserState *parser, U8 *function_name);
I64 parser_calculate_variable_address(ParserState *parser, U8 *variable_name);
I64 parser_calculate_relative_address(ParserState *parser, I64 from_address, I64 to_address);
void parser_initialize_address_space(ParserState *parser);
void parser_initialize_builtin_functions(ParserState *parser);

/* Assembly integration */
Bool parser_is_assembly_token(TokenType token);
Bool parser_is_assembly_register_token(TokenType token);
Bool parser_is_assembly_opcode_token(TokenType token);
X86Register parser_get_assembly_register(TokenType token, U8 *name);
U8* parser_get_assembly_opcode(TokenType token, U8 *name);

#endif /* PARSER_H */
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
    NODE_DO_WHILE_STMT,
    NODE_FOR_STMT,
    NODE_SWITCH,
    NODE_CASE,
    NODE_RANGE_CASE,      /* Range case (case 4...7:) */
    NODE_NULL_CASE,       /* Null case (case: with auto-increment) */
    NODE_DEFAULT,
    NODE_BREAK,
    NODE_CONTINUE,
    NODE_GOTO,
    NODE_LABEL,
    NODE_CONDITIONAL,
    NODE_SUB_SWITCH,      /* Sub-switch with start/end */
    NODE_START_BLOCK,     /* start: block */
    NODE_END_BLOCK,       /* end: block */
    
    /* Range comparison nodes */
    NODE_RANGE_COMPARISON, /* Range comparison (5<i<j+1<20) */
    
    /* Array nodes */
    NODE_ARRAY_INIT,
    
    /* Pointer nodes */
    NODE_POINTER_DEREF,
    NODE_ADDRESS_OF,
    
    /* Class/struct nodes */
    
    /* Sub-int access nodes */
    NODE_SUB_INT_ACCESS,     /* Sub-integer access (i.u16[1]) */
    NODE_UNION_MEMBER_ACCESS, /* Union member access with array notation */
    NODE_TYPE_PREFIXED_UNION, /* Type-prefixed union declaration */
    
    /* Function feature nodes */
    NODE_VARARGS,            /* Variable arguments (...) */
    NODE_DEFAULT_ARG,        /* Default argument value */
    NODE_FUNC_CALL_NO_PARENS, /* Function call without parentheses */
    NODE_INLINE_ASM,         /* Inline assembly block */
    NODE_REG_DIRECTIVE,      /* Register directive (reg/noreg) */
    NODE_TRY_BLOCK,          /* Try block */
    NODE_CATCH_BLOCK,        /* Catch block */
    NODE_THROW_STMT,         /* Throw statement */
    NODE_TYPE_INFERENCE,     /* Type inference */
    NODE_MULTI_CHAR_CONST,   /* Multi-character constant */
    NODE_ENHANCED_CAST,      /* Enhanced type casting */
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
    BINOP_DOT_DOT,    /* .. */
    
    /* Pointer arithmetic operators */
    BINOP_PTR_ADD,    /* Pointer addition */
    BINOP_PTR_SUB,    /* Pointer subtraction */
    
    /* HolyC string formatting */
    BINOP_COMMA       /* Comma operator (for HolyC string formatting) */
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
        
        /* For statement */
        struct {
            struct ASTNode *init;      /* Initialization expression */
            struct ASTNode *condition; /* Condition expression */
            struct ASTNode *increment; /* Increment expression */
            struct ASTNode *body;      /* Loop body */
        } for_stmt;
        
        /* Do-while statement */
        struct {
            struct ASTNode *body;      /* Loop body */
            struct ASTNode *condition; /* Condition expression */
        } do_while_stmt;
        
        /* While statement */
        struct {
            struct ASTNode *condition; /* Condition expression */
            struct ASTNode *body_stmt; /* Loop body */
        } while_stmt;
        
        /* Conditional expression (ternary operator) */
        struct {
            struct ASTNode *condition; /* Condition expression */
            struct ASTNode *true_expr; /* True expression */
            struct ASTNode *false_expr; /* False expression */
        } conditional;
        
        /* Switch statement */
        struct {
            struct ASTNode *expression; /* Switch expression */
            struct ASTNode *cases;      /* List of case statements */
            struct ASTNode *default_case; /* Default case (optional) */
            Bool nobounds;              /* No bounds checking (switch [expr]) */
        } switch_stmt;
        
        /* Case statement */
        struct {
            struct ASTNode *value;      /* Case value (NULL for default) */
            struct ASTNode *range_start; /* Range start (for case 5...10) */
            struct ASTNode *range_end;   /* Range end (for case 5...10) */
            struct ASTNode *body;       /* Case body statements */
            Bool is_range;              /* True if this is a range case */
            Bool is_default;            /* True if this is default case */
            Bool is_null_case;          /* True if this is a null case (auto-increment) */
            I64 auto_increment_value;   /* Value for auto-increment null cases */
        } case_stmt;
        
        /* Range case statement */
        struct {
            struct ASTNode *start_value; /* Range start value */
            struct ASTNode *end_value;   /* Range end value */
            struct ASTNode *body;        /* Case body statements */
        } range_case;
        
        /* Null case statement */
        struct {
            I64 auto_value;             /* Auto-increment value */
            struct ASTNode *body;       /* Case body statements */
        } null_case;
        
        /* Sub-switch statement */
        struct {
            struct ASTNode *parent_switch; /* Parent switch statement */
            struct ASTNode *start_block;   /* start: block */
            struct ASTNode *end_block;     /* end: block */
            struct ASTNode *cases;         /* Cases within sub-switch */
        } sub_switch;
        
        /* Start/End block */
        struct {
            struct ASTNode *statements;    /* Statements in the block */
            Bool is_start;                 /* True if start block, false if end block */
        } start_end_block;
        
        /* Range comparison */
        struct {
            struct ASTNode *expressions;   /* List of expressions in the range */
            struct ASTNode *operators;     /* List of comparison operators */
            I64 expression_count;          /* Number of expressions */
        } range_comparison;
        
        /* Goto statement */
        struct {
            U8 *label_name;             /* Target label name */
            I64 jump_target;            /* Jump target address (for codegen) */
        } goto_stmt;
        
        /* Label statement */
        struct {
            U8 *label_name;             /* Label name */
            Bool is_exported;           /* True if exported (label::) */
            Bool is_local;              /* True if local (@@label:) */
            I64 label_address;          /* Label address (for codegen) */
        } label_stmt;
        
        /* Array access */
        struct {
            struct ASTNode *array;      /* Array expression */
            struct ASTNode *index;      /* Index expression */
        } array_access;
        
        /* Array initialization */
        struct {
            struct ASTNode *elements;   /* List of initialization elements */
            I64 element_count;          /* Number of elements */
        } array_init;
        
        /* Pointer dereference */
        struct {
            struct ASTNode *pointer;    /* Pointer expression */
        } pointer_deref;
        
        /* Address-of */
        struct {
            struct ASTNode *variable;   /* Variable expression */
        } address_of;
        
        /* Class/struct definition */
        struct {
            U8 *class_name;             /* Class name */
            U8 *base_class;             /* Base class name (for inheritance) */
            struct ASTNode *members;    /* List of member declarations */
            I64 member_count;           /* Number of members */
            Bool is_public;             /* True if public class */
            Bool is_union;              /* True if union */
        } class_def;
        
        /* Member access */
        struct {
            struct ASTNode *object;     /* Object expression */
            U8 *member_name;            /* Member name */
        } member_access;
        
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
            Bool is_array;    /* True if this is an array */
            struct ASTNode *array_size; /* Array size expression (NULL for dynamic arrays) */
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
            SchismTokenType type;   /* Token type (TK_TYPE_I64, etc.) */
            U8 *type_name;    /* Type name string */
            I64 size;         /* Type size in bytes */
            Bool is_signed;   /* Signed type */
            Bool is_floating; /* Floating point type */
            Bool is_pointer;  /* Pointer type */
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
        
        /* Boolean literal */
        struct {
            Bool value;        /* Boolean value */
        } boolean;
        
        /* Sub-int access (i.u16[1]) */
        struct {
            struct ASTNode *base_object;  /* Base object (i) */
            U8 *member_type;              /* Member type (u16) */
            struct ASTNode *index;        /* Index expression ([1]) */
            I64 member_size;              /* Size of member type in bytes */
            I64 member_offset;            /* Offset within the base object */
            Bool is_signed;               /* Whether member type is signed */
        } sub_int_access;
        
        /* Union member access with array notation */
        struct {
            struct ASTNode *union_object; /* Union object */
            U8 *member_name;              /* Member name */
            struct ASTNode *index;        /* Array index */
            I64 member_size;              /* Size of member type */
            I64 member_offset;            /* Offset within union */
        } union_member_access;
        
        /* Type-prefixed union declaration */
        struct {
            U8 *prefix_type;              /* Type prefix (I64i) */
            U8 *union_name;               /* Union name (I64) */
            struct ASTNode *members;      /* Union members */
            I64 member_count;             /* Number of members */
            Bool is_public;               /* Public union */
        } type_prefixed_union;
        
        /* Variable arguments */
        struct {
            U8 *type;                     /* Argument type */
            I64 arg_index;                /* Argument index */
            Bool is_va_list;              /* Uses va_list */
        } varargs;
        
        /* Default argument */
        struct {
            struct ASTNode *parameter;    /* Parameter node */
            struct ASTNode *default_value; /* Default value expression */
        } default_arg;
        
        /* Function call without parentheses */
        struct {
            U8 *name;                     /* Function name */
            struct ASTNode *arguments;    /* Argument list */
            U8 *return_type;              /* Return type */
        } func_call_no_parens;
        
        /* Inline assembly block */
        struct {
            struct ASTNode *instructions; /* Assembly instructions */
            Bool is_volatile;             /* Volatile assembly */
            I64 input_count;              /* Number of input operands */
            I64 output_count;             /* Number of output operands */
            I64 clobber_count;            /* Number of clobbered registers */
            U8 **input_ops;               /* Input operands */
            U8 **output_ops;              /* Output operands */
            U8 **clobber_ops;             /* Clobbered registers */
        } inline_asm;
        
        /* Register directive */
        struct {
            Bool is_reg;                  /* True for reg, false for noreg */
            U8 *function_name;            /* Function name (if applicable) */
        } reg_directive;
        
        /* Try block */
        struct {
            struct ASTNode *try_body;     /* Try block body */
            struct ASTNode *catch_blocks; /* Catch blocks */
            I64 catch_count;              /* Number of catch blocks */
        } try_block;
        
        /* Catch block */
        struct {
            U8 *exception_type;           /* Exception type */
            U8 *exception_name;           /* Exception variable name */
            struct ASTNode *catch_body;   /* Catch block body */
        } catch_block;
        
        /* Throw statement */
        struct {
            struct ASTNode *exception;    /* Exception expression */
            U8 *exception_type;           /* Exception type */
        } throw_stmt;
        
        /* Type inference */
        struct {
            struct ASTNode *expression;   /* Expression to infer type from */
            U8 *inferred_type;            /* Inferred type */
            Bool is_auto;                 /* Uses auto keyword */
        } type_inference;
        
        /* Multi-character constant */
        struct {
            U8 *value;                    /* Character sequence */
            I64 length;                   /* Number of characters */
            I64 int_value;                /* Integer value */
        } multi_char_const;
        
        /* Enhanced type casting */
        struct {
            U8 *target_type;              /* Target type */
            struct ASTNode *expression;   /* Expression to cast */
            Bool is_explicit;             /* Explicit cast */
            Bool is_const_cast;           /* Const cast */
            Bool is_reinterpret_cast;     /* Reinterpret cast */
        } enhanced_cast;
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
    
    /* Error recovery state */
    Bool recovery_mode;       /* Whether in error recovery mode */
    I64 recovery_depth;       /* Depth of recovery nesting */
    I64 max_recovery_depth;   /* Maximum recovery depth allowed */
    I64 recovery_attempts;    /* Number of recovery attempts */
    I64 max_recovery_attempts; /* Maximum recovery attempts per error */
    Bool recovery_successful; /* Whether last recovery was successful */
    
    /* Recovery state tracking */
    struct {
        I64 saved_buffer_pos;
        I64 saved_buffer_line;
        I64 saved_buffer_column;
        SchismTokenType saved_current_token;
        U8 *saved_token_value;
        I64 saved_token_length;
        I64 saved_error_count;
        I64 saved_warning_count;
    } recovery_state;
    
    /* Position tracking for lookahead */
    I64 saved_buffer_pos;     /* Saved buffer position */
    I64 saved_buffer_line;    /* Saved buffer line */
    I64 saved_buffer_column;  /* Saved buffer column */
    SchismTokenType saved_current_token; /* Saved current token */
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
ASTNode* parse_do_while_statement(ParserState *parser);
ASTNode* parse_for_statement(ParserState *parser);
ASTNode* parse_switch_statement(ParserState *parser);
ASTNode* parse_case_statement(ParserState *parser);
ASTNode* parse_default_statement(ParserState *parser);
ASTNode* parse_start_end_block(ParserState *parser, Bool is_start);
ASTNode* parse_range_comparison(ParserState *parser, ASTNode *first_expr);
ASTNode* parse_return_statement(ParserState *parser);
ASTNode* parse_break_statement(ParserState *parser);
ASTNode* parse_continue_statement(ParserState *parser);
ASTNode* parse_goto_statement(ParserState *parser);
ASTNode* parse_label_statement(ParserState *parser);
ASTNode* parse_block_statement(ParserState *parser);

/* Expression parsing */
ASTNode* parse_expression(ParserState *parser);
ASTNode* parse_comma_expression(ParserState *parser);
ASTNode* parse_assignment_expression(ParserState *parser);
ASTNode* parse_conditional_expression(ParserState *parser);
ASTNode* parse_logical_or_expression(ParserState *parser);
ASTNode* parse_logical_xor_expression(ParserState *parser);
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
ASTNode* parse_array_initializer(ParserState *parser);
ASTNode* parse_pointer_dereference(ParserState *parser);
ASTNode* parse_address_of(ParserState *parser);
ASTNode* parse_class_definition(ParserState *parser);
ASTNode* parse_member_access(ParserState *parser);
ASTNode* parse_pointer_arithmetic(ParserState *parser);

/* Sub-int access parsing functions */
ASTNode* parse_sub_int_access(ParserState *parser);
ASTNode* parse_union_member_access(ParserState *parser);
ASTNode* parse_type_prefixed_union(ParserState *parser);
Bool is_sub_int_access_pattern(ParserState *parser);

/* Enhanced function feature parsing */
ASTNode* parse_variable_arguments(ParserState *parser);
ASTNode* parse_default_argument(ParserState *parser);
ASTNode* parse_function_call_no_parens(ParserState *parser);
ASTNode* parse_inline_assembly_block(ParserState *parser);
ASTNode* parse_register_directive(ParserState *parser);
ASTNode* parse_try_block(ParserState *parser);
ASTNode* parse_catch_block(ParserState *parser);
ASTNode* parse_throw_statement(ParserState *parser);
ASTNode* parse_type_inference(ParserState *parser);
ASTNode* parse_multi_character_constant(ParserState *parser);
ASTNode* parse_enhanced_type_cast(ParserState *parser);

/* Parser utility functions */
Bool parser_is_function_defined_in_scope(ParserState *parser, U8 *name);

/* AST node management */
ASTNode* ast_node_new(ASTNodeType type, I64 line, I64 column);
void ast_node_free(ASTNode *node);
void ast_node_add_child(ASTNode *parent, ASTNode *child);
void ast_node_add_sibling(ASTNode *node, ASTNode *sibling);

/* Utility functions */
SchismTokenType parser_expect_token(ParserState *parser, SchismTokenType expected);
Bool parser_match_token(ParserState *parser, SchismTokenType token);
SchismTokenType parser_current_token(ParserState *parser);
SchismTokenType parser_next_token(ParserState *parser);
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
void parser_expected_error(ParserState *parser, SchismTokenType expected, SchismTokenType found);

/* Error recovery */
typedef enum {
    RECOVERY_SKIP_TO_SEMICOLON,    /* Skip to next semicolon */
    RECOVERY_SKIP_TO_BRACE,        /* Skip to matching brace */
    RECOVERY_SKIP_TO_PAREN,        /* Skip to matching parenthesis */
    RECOVERY_SKIP_TO_KEYWORD,      /* Skip to specific keyword */
    RECOVERY_SKIP_TO_NEWLINE,      /* Skip to next newline */
    RECOVERY_INSERT_TOKEN,         /* Insert missing token */
    RECOVERY_DELETE_TOKEN,         /* Delete erroneous token */
    RECOVERY_REPLACE_TOKEN,        /* Replace token with expected one */
    RECOVERY_RESTART_STATEMENT,    /* Restart current statement */
    RECOVERY_RESTART_FUNCTION,     /* Restart current function */
    RECOVERY_RESTART_BLOCK         /* Restart current block */
} ErrorRecoveryStrategy;

typedef struct {
    ErrorRecoveryStrategy strategy;
    SchismTokenType target_token;        /* For keyword/brace/paren recovery */
    U8 *target_keyword;           /* For keyword recovery */
    Bool insert_semicolon;        /* Whether to insert semicolon */
    Bool skip_current_token;      /* Whether to skip current token */
    I64 max_skip_tokens;          /* Maximum tokens to skip */
} ErrorRecoveryInfo;

/* Error recovery functions */
Bool parser_attempt_error_recovery(ParserState *parser, ErrorRecoveryInfo *recovery);
Bool parser_skip_to_semicolon(ParserState *parser);
Bool parser_skip_to_brace(ParserState *parser, SchismTokenType open_brace, SchismTokenType close_brace);
Bool parser_skip_to_keyword(ParserState *parser, U8 *keyword);
Bool parser_skip_to_newline(ParserState *parser);
Bool parser_insert_missing_token(ParserState *parser, SchismTokenType token);
Bool parser_delete_current_token(ParserState *parser);
Bool parser_replace_current_token(ParserState *parser, SchismTokenType new_token);
Bool parser_restart_statement(ParserState *parser);
Bool parser_restart_function(ParserState *parser);
Bool parser_restart_block(ParserState *parser);

/* Error recovery with context */
Bool parser_recover_from_syntax_error(ParserState *parser, U8 *context);
Bool parser_recover_from_missing_token(ParserState *parser, SchismTokenType expected);
Bool parser_recover_from_unexpected_token(ParserState *parser, SchismTokenType unexpected);
Bool parser_recover_from_incomplete_statement(ParserState *parser);
Bool parser_recover_from_incomplete_expression(ParserState *parser);
Bool parser_recover_from_incomplete_function(ParserState *parser);
Bool parser_recover_from_incomplete_block(ParserState *parser);

/* Error recovery state management */
void parser_init_recovery_state(ParserState *parser);
void parser_save_recovery_state(ParserState *parser);
void parser_restore_recovery_state(ParserState *parser);
Bool parser_can_recover(ParserState *parser);
void parser_set_recovery_mode(ParserState *parser, Bool enabled);
Bool parser_is_in_recovery_mode(ParserState *parser);

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
Bool parser_is_assembly_token(SchismTokenType token);
Bool parser_is_assembly_register_token(SchismTokenType token);
Bool parser_is_assembly_opcode_token(SchismTokenType token);
X86Register parser_get_assembly_register(SchismTokenType token, U8 *name);
U8* parser_get_assembly_opcode(SchismTokenType token, U8 *name);

#endif /* PARSER_H */
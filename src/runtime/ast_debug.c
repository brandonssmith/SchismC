/*
 * AST Debugging Functions for SchismC
 * Pretty-printing and visualization of Abstract Syntax Trees
 */

#include "debug.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* AST Node Type Names */
static const char* ast_node_type_names[] = {
    "UNKNOWN",
    "PROGRAM",
    "FUNCTION",
    "VARIABLE",
    "ASSIGNMENT",
    "BINARY_OP",
    "UNARY_OP",
    "CALL",
    "IF",
    "WHILE",
    "FOR",
    "RETURN",
    "BLOCK",
    "IDENTIFIER",
    "INTEGER",
    "FLOAT",
    "STRING",
    "CHAR",
    "BOOLEAN",
    "ASM_BLOCK",
    "ASM_INSTRUCTION",
    "ASM_OPERAND",
    "ASM_REGISTER",
    "ASM_MEMORY",
    "ASM_IMMEDIATE",
    "ASM_LABEL",
    "ASM_DIRECTIVE",
    "RANGE_EXPR",
    "DOLLAR_EXPR",
    "CLASS_DEF",
    "UNION_DEF",
    "PUBLIC_DECL",
    "EXTERN_DECL",
    "IMPORT_DECL",
    "TRY_CATCH",
    "THROW",
    "TYPE_DECL",
    "TYPE_SPECIFIER",
    "TYPE_CAST",
    "TYPE_REF",
    "TYPE_DEREF",
    "ARRAY_ACCESS",
    "MEMBER_ACCESS",
    "POINTER_ARITH",
    "IF_STMT",
    "WHILE_STMT",
    "DO_WHILE_STMT",
    "FOR_STMT",
    "SWITCH",
    "CASE",
    "DEFAULT",
    "BREAK",
    "CONTINUE",
    "GOTO",
    "LABEL",
    "CONDITIONAL",
    "ARRAY_INIT",
    "POINTER_DEREF",
    "ADDRESS_OF"
};

/* Binary Operator Names */
static const char* binary_op_names[] = {
    "UNKNOWN",
    "ADD",           /* + */
    "SUB",           /* - */
    "MUL",           /* * */
    "DIV",           /* / */
    "MOD",           /* % */
    "EQ",            /* == */
    "NE",            /* != */
    "LT",            /* < */
    "LE",            /* <= */
    "GT",            /* > */
    "GE",            /* >= */
    "AND",           /* && */
    "OR",            /* || */
    "BIT_AND",       /* & */
    "BIT_OR",        /* | */
    "BIT_XOR",       /* ^ */
    "SHL",           /* << */
    "SHR",           /* >> */
    "ASSIGN",        /* = */
    "ADD_ASSIGN",    /* += */
    "SUB_ASSIGN",    /* -= */
    "MUL_ASSIGN",    /* *= */
    "DIV_ASSIGN",    /* /= */
    "MOD_ASSIGN",    /* %= */
    "AND_ASSIGN",    /* &= */
    "OR_ASSIGN",     /* |= */
    "XOR_ASSIGN",    /* ^= */
    "SHL_ASSIGN",    /* <<= */
    "SHR_ASSIGN"     /* >>= */
};

/* Unary Operator Names */
static const char* unary_op_names[] = {
    "UNKNOWN",
    "PLUS",          /* + */
    "MINUS",         /* - */
    "NOT",           /* ! */
    "BIT_NOT",       /* ~ */
    "PRE_INC",       /* ++ */
    "POST_INC",      /* ++ */
    "PRE_DEC",       /* -- */
    "POST_DEC"       /* -- */
};

/* Get node type name */
static const char* get_node_type_name(ASTNodeType type) {
    if (type >= 0 && type < sizeof(ast_node_type_names) / sizeof(ast_node_type_names[0])) {
        return ast_node_type_names[type];
    }
    return "UNKNOWN";
}

/* Get binary operator name */
static const char* get_binary_op_name(BinaryOpType op) {
    if (op >= 0 && op < sizeof(binary_op_names) / sizeof(binary_op_names[0])) {
        return binary_op_names[op];
    }
    return "UNKNOWN";
}

/* Get unary operator name */
static const char* get_unary_op_name(UnaryOpType op) {
    if (op >= 0 && op < sizeof(unary_op_names) / sizeof(unary_op_names[0])) {
        return unary_op_names[op];
    }
    return "UNKNOWN";
}

/* Print indentation */
static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

/* Print AST node */
void debug_ast_print_node(ASTNode *node, int indent) {
    if (!node) {
        print_indent(indent);
        printf("NULL\n");
        return;
    }
    
    print_indent(indent);
    printf("%s", get_node_type_name(node->type));
    
    /* Print line and column if available */
    if (node->line > 0) {
        printf(" [line %lld", (long long)node->line);
        if (node->column > 0) {
            printf(", col %lld", (long long)node->column);
        }
        printf("]");
    }
    
    printf("\n");
}

/* Print AST children */
void debug_ast_print_children(ASTNode *node, int indent) {
    if (!node) return;
    
    ASTNode *child = node->children;
    while (child) {
        debug_ast_print(child, indent + 1);
        child = child->next;
    }
}

/* Print complete AST */
void debug_ast_print(ASTNode *node, int indent) {
    if (!node) {
        print_indent(indent);
        printf("NULL\n");
        return;
    }
    
    debug_ast_print_node(node, indent);
    debug_ast_print_children(node, indent);
}

/* Print AST statistics */
void debug_ast_print_statistics(ASTNode *node) {
    if (!node) {
        printf("AST Statistics: NULL tree\n");
        return;
    }
    
    U64 node_count = 0;
    U64 function_count = 0;
    U64 variable_count = 0;
    U64 expression_count = 0;
    U64 statement_count = 0;
    U64 max_depth = 0;
    
    debug_ast_count_nodes(node, 0, &node_count, &function_count, &variable_count, 
                         &expression_count, &statement_count, &max_depth);
    
    printf("\n=== AST Statistics ===\n");
    printf("Total nodes: %llu\n", (unsigned long long)node_count);
    printf("Functions: %llu\n", (unsigned long long)function_count);
    printf("Variables: %llu\n", (unsigned long long)variable_count);
    printf("Expressions: %llu\n", (unsigned long long)expression_count);
    printf("Statements: %llu\n", (unsigned long long)statement_count);
    printf("Maximum depth: %llu\n", (unsigned long long)max_depth);
    printf("======================\n");
}

/* Count nodes in AST */
void debug_ast_count_nodes(ASTNode *node, int depth, U64 *node_count, U64 *function_count, 
                          U64 *variable_count, U64 *expression_count, U64 *statement_count, U64 *max_depth) {
    if (!node) return;
    
    (*node_count)++;
    if (depth > *max_depth) {
        *max_depth = depth;
    }
    
    switch (node->type) {
        case NODE_FUNCTION:
            (*function_count)++;
            break;
        case NODE_VARIABLE:
            (*variable_count)++;
            break;
        case NODE_BINARY_OP:
        case NODE_UNARY_OP:
        case NODE_CALL:
        case NODE_IDENTIFIER:
        case NODE_INTEGER:
        case NODE_FLOAT:
        case NODE_STRING:
        case NODE_CHAR:
        case NODE_BOOLEAN:
        case NODE_ARRAY_ACCESS:
        case NODE_MEMBER_ACCESS:
        case NODE_POINTER_DEREF:
        case NODE_ADDRESS_OF:
        case NODE_CONDITIONAL:
            (*expression_count)++;
            break;
        case NODE_IF:
        case NODE_WHILE:
        case NODE_FOR:
        case NODE_RETURN:
        case NODE_BLOCK:
        case NODE_ASSIGNMENT:
        case NODE_BREAK:
        case NODE_CONTINUE:
        case NODE_GOTO:
        case NODE_LABEL:
        case NODE_SWITCH:
        case NODE_CASE:
        case NODE_DEFAULT:
            (*statement_count)++;
            break;
        default:
            break;
    }
    
    /* Recursively count children */
    ASTNode *child = node->children;
    while (child) {
        debug_ast_count_nodes(child, depth + 1, node_count, function_count, 
                             variable_count, expression_count, statement_count, max_depth);
        child = child->next;
    }
}

/* Print AST in JSON format */
void debug_ast_print_json(ASTNode *node, int indent) {
    if (!node) {
        print_indent(indent);
        printf("null");
        return;
    }
    
    print_indent(indent);
    printf("{\n");
    
    print_indent(indent + 1);
    printf("\"type\": \"%s\",\n", get_node_type_name(node->type));
    
    /* Print basic node information */
    print_indent(indent + 1);
    printf("\"type_id\": %d,\n", node->type);
    
    if (node->line > 0) {
        print_indent(indent + 1);
        printf("\"line\": %lld,\n", (long long)node->line);
    }
    
    if (node->column > 0) {
        print_indent(indent + 1);
        printf("\"column\": %lld,\n", (long long)node->column);
    }
    
    /* Print children */
    if (node->children) {
        print_indent(indent + 1);
        printf("\"children\": [\n");
        
        ASTNode *child = node->children;
        bool first = true;
        while (child) {
            if (!first) {
                printf(",\n");
            }
            debug_ast_print_json(child, indent + 2);
            first = false;
            child = child->next;
        }
        
        printf("\n");
        print_indent(indent + 1);
        printf("]\n");
    }
    
    print_indent(indent);
    printf("}");
}

/* Print AST in DOT format for graphviz */
void debug_ast_print_dot(ASTNode *node, int *node_id) {
    if (!node) return;
    
    int current_id = (*node_id)++;
    
    /* Print node */
    printf("  node%d [label=\"%s", current_id, get_node_type_name(node->type));
    
    /* Print basic node information */
    printf("\\nType: %d", node->type);
    
    printf("\"];\n");
    
    /* Print edges to children */
    ASTNode *child = node->children;
    while (child) {
        int child_id = *node_id;
        printf("  node%d -> node%d;\n", current_id, child_id);
        debug_ast_print_dot(child, node_id);
        child = child->next;
    }
}

/* Print complete AST in DOT format */
void debug_ast_print_dot_complete(ASTNode *node) {
    printf("digraph AST {\n");
    printf("  rankdir=TB;\n");
    printf("  node [shape=box, style=filled, fillcolor=lightblue];\n");
    
    int node_id = 0;
    debug_ast_print_dot(node, &node_id);
    
    printf("}\n");
}

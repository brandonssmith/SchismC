/*
 * Symbol Table Debugging Functions for SchismC
 * Pretty-printing and inspection of symbol tables and scopes
 */

#include "debug.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Print indentation */
static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

/* Print symbol table scope */
void debug_symbol_table_print_scope(ScopeLevel *scope, int depth) {
    if (!scope) {
        print_indent(depth);
        printf("NULL scope\n");
        return;
    }
    
    print_indent(depth);
    printf("Scope Level %d:\n", depth);
    print_indent(depth + 1);
    printf("Function scope: %s\n", scope->is_function_scope ? "true" : "false");
    print_indent(depth + 1);
    printf("Block scope: %s\n", scope->is_block_scope ? "true" : "false");
    
    /* Print basic scope information */
    print_indent(depth + 1);
    printf("Scope address: %p\n", scope);
}

/* Print complete symbol table */
void debug_symbol_table_print(ParserState *parser) {
    if (!parser) {
        printf("Symbol Table: NULL parser\n");
        return;
    }
    
    printf("\n=== Symbol Table ===\n");
    printf("Parser state: %p\n", parser);
    printf("===================\n");
}

/* Print symbol table statistics */
void debug_symbol_table_print_statistics(ParserState *parser) {
    if (!parser) {
        printf("Symbol Table Statistics: NULL parser\n");
        return;
    }
    
    printf("\n=== Symbol Table Statistics ===\n");
    printf("Parser state: %p\n", parser);
    printf("================================\n");
}

/* Count symbols in symbol table */
void debug_symbol_table_count_symbols(ScopeLevel *scope, int depth, U64 *total_scopes, 
                                     U64 *total_variables, U64 *total_functions, U64 *max_depth) {
    if (!scope) return;
    (*total_scopes)++;
}

/* Print symbol table in JSON format */
void debug_symbol_table_print_json(ScopeLevel *scope, int indent) {
    if (!scope) {
        print_indent(indent);
        printf("null");
        return;
    }
    
    print_indent(indent);
    printf("{\n");
    print_indent(indent + 1);
    printf("\"scope\": \"%p\"\n", scope);
    print_indent(indent);
    printf("}");
}

/* Print symbol table in DOT format for graphviz */
void debug_symbol_table_print_dot(ScopeLevel *scope, int *scope_id) {
    if (!scope) return;
    int current_id = (*scope_id)++;
    printf("  scope%d [label=\"Scope %p\"];\n", current_id, scope);
}

/* Print complete symbol table in DOT format */
void debug_symbol_table_print_dot_complete(ParserState *parser) {
    printf("digraph SymbolTable {\n  node [label=\"Symbol Table\"];\n}\n");
}

/* Find and print symbol by name */
void debug_symbol_table_find_symbol(ParserState *parser, const char *name) {
    if (!parser || !name) {
        printf("Find Symbol: Invalid parameters\n");
        return;
    }
    
    printf("\n=== Finding Symbol: %s ===\n", name);
    printf("Parser: %p\n", parser);
    printf("===============================\n");
}

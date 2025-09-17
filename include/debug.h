/*
 * Debug and Logging System for SchismC
 * Centralized debugging infrastructure with configurable levels and output
 */

#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include "core_structures.h"
#include "parser.h"

/* Debug Levels */
typedef enum {
    DEBUG_NONE = 0,        /* No debug output */
    DEBUG_ERROR = 1,       /* Errors only */
    DEBUG_WARNING = 2,     /* Warnings and errors */
    DEBUG_INFO = 3,        /* Informational messages */
    DEBUG_VERBOSE = 4,     /* Verbose output */
    DEBUG_TRACE = 5,       /* Full tracing */
    DEBUG_ALL = 6          /* Everything */
} DebugLevel;

/* Debug Categories */
typedef enum {
    DEBUG_CAT_GENERAL = 0,
    DEBUG_CAT_LEXER = 1,
    DEBUG_CAT_PARSER = 2,
    DEBUG_CAT_AST = 3,
    DEBUG_CAT_SYMBOL_TABLE = 4,
    DEBUG_CAT_INTERMEDIATE = 5,
    DEBUG_CAT_ASSEMBLY = 6,
    DEBUG_CAT_AOT = 7,
    DEBUG_CAT_MASM = 8,
    DEBUG_CAT_OPTIMIZATION = 9,
    DEBUG_CAT_REGISTER = 10,
    DEBUG_CAT_MEMORY = 11,
    DEBUG_CAT_PERFORMANCE = 12,
    DEBUG_CAT_MAX
} DebugCategory;

/* Debug Context Structure */
typedef struct {
    DebugLevel level;                    /* Current debug level */
    DebugLevel category_levels[DEBUG_CAT_MAX]; /* Per-category levels */
    FILE *output_file;                   /* Output file (NULL = stdout) */
    Bool color_output;                   /* Enable colored output */
    Bool timestamp_output;               /* Include timestamps */
    Bool show_category;                  /* Show debug category */
    Bool show_location;                  /* Show file:line location */
    Bool show_thread_id;                 /* Show thread ID */
    char *log_file_path;                 /* Log file path */
    FILE *log_file;                      /* Log file handle */
    U64 message_count;                   /* Total message count */
    U64 error_count;                     /* Error count */
    U64 warning_count;                   /* Warning count */
    U64 info_count;                      /* Info count */
    U64 verbose_count;                   /* Verbose count */
    U64 trace_count;                     /* Trace count */
} DebugContext;

/* Global debug context */
extern DebugContext *g_debug_ctx;

/* Debug Context Management */
DebugContext* debug_context_new(void);
void debug_context_free(DebugContext *ctx);
void debug_context_set_level(DebugContext *ctx, DebugLevel level);
void debug_context_set_category_level(DebugContext *ctx, DebugCategory category, DebugLevel level);
void debug_context_set_output_file(DebugContext *ctx, FILE *file);
void debug_context_set_log_file(DebugContext *ctx, const char *path);
void debug_context_set_color(DebugContext *ctx, Bool enabled);
void debug_context_set_timestamp(DebugContext *ctx, Bool enabled);
void debug_context_set_show_category(DebugContext *ctx, Bool enabled);
void debug_context_set_show_location(DebugContext *ctx, Bool enabled);

/* Main Debug Functions */
void debug_log(DebugContext *ctx, DebugLevel level, DebugCategory category, 
               const char *file, int line, const char *function, const char *format, ...);
void debug_log_va(DebugContext *ctx, DebugLevel level, DebugCategory category,
                  const char *file, int line, const char *function, const char *format, va_list args);

/* Enhanced Error Reporting */
void debug_error_with_context(DebugContext *ctx, const char *file, int line, const char *function,
                              const char *source_file, int source_line, int source_column,
                              const char *error_type, const char *message, ...);
void debug_warning_with_context(DebugContext *ctx, const char *file, int line, const char *function,
                                const char *source_file, int source_line, int source_column,
                                const char *warning_type, const char *message, ...);
void debug_print_source_context(const char *source_file, int error_line, int context_lines);

/* Convenience Macros */
#define DEBUG_ERROR(cat, ...) \
    debug_log(g_debug_ctx, DEBUG_ERROR, cat, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#define DEBUG_WARNING(cat, ...) \
    debug_log(g_debug_ctx, DEBUG_WARNING, cat, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#define DEBUG_INFO(cat, ...) \
    debug_log(g_debug_ctx, DEBUG_INFO, cat, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#define DEBUG_VERBOSE(cat, ...) \
    debug_log(g_debug_ctx, DEBUG_VERBOSE, cat, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#define DEBUG_TRACE(cat, ...) \
    debug_log(g_debug_ctx, DEBUG_TRACE, cat, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

/* General debug macro */
#define DEBUG_GENERAL(level, ...) \
    debug_log(g_debug_ctx, level, DEBUG_CAT_GENERAL, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

/* Category-specific macros */
#define DEBUG_LEXER(level, ...) \
    debug_log(g_debug_ctx, level, DEBUG_CAT_LEXER, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#define DEBUG_PARSER(level, ...) \
    debug_log(g_debug_ctx, level, DEBUG_CAT_PARSER, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#define DEBUG_AST(level, ...) \
    debug_log(g_debug_ctx, level, DEBUG_CAT_AST, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#define DEBUG_SYMBOL_TABLE(level, ...) \
    debug_log(g_debug_ctx, level, DEBUG_CAT_SYMBOL_TABLE, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#define DEBUG_INTERMEDIATE(level, ...) \
    debug_log(g_debug_ctx, level, DEBUG_CAT_INTERMEDIATE, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#define DEBUG_ASSEMBLY(level, ...) \
    debug_log(g_debug_ctx, level, DEBUG_CAT_ASSEMBLY, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#define DEBUG_AOT(level, ...) \
    debug_log(g_debug_ctx, level, DEBUG_CAT_AOT, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#define DEBUG_MASM(level, ...) \
    debug_log(g_debug_ctx, level, DEBUG_CAT_MASM, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#define DEBUG_OPTIMIZATION(level, ...) \
    debug_log(g_debug_ctx, level, DEBUG_CAT_OPTIMIZATION, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#define DEBUG_REGISTER(level, ...) \
    debug_log(g_debug_ctx, level, DEBUG_CAT_REGISTER, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#define DEBUG_MEMORY(level, ...) \
    debug_log(g_debug_ctx, level, DEBUG_CAT_MEMORY, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#define DEBUG_PERFORMANCE(level, ...) \
    debug_log(g_debug_ctx, level, DEBUG_CAT_PERFORMANCE, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

/* Enhanced Error Reporting Macros */
#define DEBUG_ERROR_WITH_CONTEXT(source_file, source_line, source_column, error_type, ...) \
    debug_error_with_context(g_debug_ctx, __FILE__, __LINE__, __FUNCTION__, \
                            source_file, source_line, source_column, error_type, __VA_ARGS__)

#define DEBUG_WARNING_WITH_CONTEXT(source_file, source_line, source_column, warning_type, ...) \
    debug_warning_with_context(g_debug_ctx, __FILE__, __LINE__, __FUNCTION__, \
                              source_file, source_line, source_column, warning_type, __VA_ARGS__)

/* Utility Functions */
const char* debug_level_to_string(DebugLevel level);
const char* debug_category_to_string(DebugCategory category);
void debug_print_statistics(DebugContext *ctx);
void debug_reset_statistics(DebugContext *ctx);
void debug_flush(DebugContext *ctx);

/* Performance Timing */
typedef struct {
    clock_t start_time;
    clock_t end_time;
    const char *name;
    Bool is_running;
} DebugTimer;

DebugTimer* debug_timer_start(const char *name);
void debug_timer_end(DebugTimer *timer);
void debug_timer_print(DebugTimer *timer);
void debug_timer_free(DebugTimer *timer);

/* Memory Debugging */
void debug_memory_alloc(const char *file, int line, const char *function, void *ptr, size_t size);
void debug_memory_free(const char *file, int line, const char *function, void *ptr);
void debug_memory_print_statistics(void);

/* AST Debugging */
void debug_ast_print(ASTNode *node, int indent);
void debug_ast_print_node(ASTNode *node, int indent);
void debug_ast_print_children(ASTNode *node, int indent);
void debug_ast_print_statistics(ASTNode *node);
void debug_ast_count_nodes(ASTNode *node, int depth, U64 *node_count, U64 *function_count, 
                          U64 *variable_count, U64 *expression_count, U64 *statement_count, U64 *max_depth);
void debug_ast_print_json(ASTNode *node, int indent);
void debug_ast_print_dot(ASTNode *node, int *node_id);
void debug_ast_print_dot_complete(ASTNode *node);

/* Symbol Table Debugging */
void debug_symbol_table_print(ParserState *parser);
void debug_symbol_table_print_scope(ScopeLevel *scope, int depth);
void debug_symbol_table_print_statistics(ParserState *parser);
void debug_symbol_table_count_symbols(ScopeLevel *scope, int depth, U64 *total_scopes, 
                                     U64 *total_variables, U64 *total_functions, U64 *max_depth);
void debug_symbol_table_print_json(ScopeLevel *scope, int indent);
void debug_symbol_table_print_dot(ScopeLevel *scope, int *scope_id);
void debug_symbol_table_print_dot_complete(ParserState *parser);
void debug_symbol_table_find_symbol(ParserState *parser, const char *name);

/* Assembly Debugging */
void debug_assembly_print_instruction(CIntermediateCode *ic);
void debug_assembly_print_register_allocation(CCmpCtrl *cc);
void debug_assembly_print_memory_layout(CCmpCtrl *cc);
void debug_assembly_print_argument(CAsmArg *arg, const char *name);
void debug_assembly_print_statistics(CCmpCtrl *cc);
void debug_assembly_print_intel_syntax(CIntermediateCode *ic);
void debug_assembly_print_att_syntax(CIntermediateCode *ic);

/* Command Line Debug Options */
typedef struct {
    Bool verbose;                    /* -v, --verbose */
    Bool trace;                      /* --trace */
    DebugLevel level;                /* --debug-level */
    char *log_file;                  /* --log-file */
    Bool color;                      /* --color */
    Bool no_timestamp;               /* --no-timestamp */
    Bool show_category;              /* --show-category */
    Bool show_location;              /* --show-location */
    char *debug_categories;          /* --debug-categories */
} DebugOptions;

DebugOptions* debug_options_parse(int argc, char *argv[]);
void debug_options_apply(DebugContext *ctx, DebugOptions *options);
void debug_options_free(DebugOptions *options);

/* Initialization and Cleanup */
void debug_system_init(void);
void debug_system_cleanup(void);

#endif /* DEBUG_H */

/*
 * Debug and Logging System Implementation for SchismC
 * Centralized debugging infrastructure with configurable levels and output
 */

#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

/* strdup implementation for systems that don't have it */
char* strdup(const char* s) {
    size_t len = strlen(s) + 1;
    char* dup = malloc(len);
    if (dup) {
        memcpy(dup, s, len);
    }
    return dup;
}

/* Global debug context */
DebugContext *g_debug_ctx = NULL;

/* Color codes for terminal output */
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"
#define COLOR_BOLD    "\033[1m"

/* Debug Context Management */
DebugContext* debug_context_new(void) {
    DebugContext *ctx = (DebugContext*)malloc(sizeof(DebugContext));
    if (!ctx) return NULL;
    
    /* Initialize with defaults */
    ctx->level = DEBUG_INFO;
    ctx->output_file = stdout;
    ctx->color_output = true;
    ctx->timestamp_output = true;
    ctx->show_category = true;
    ctx->show_location = false;
    ctx->show_thread_id = false;
    ctx->log_file_path = NULL;
    ctx->log_file = NULL;
    ctx->message_count = 0;
    ctx->error_count = 0;
    ctx->warning_count = 0;
    ctx->info_count = 0;
    ctx->verbose_count = 0;
    ctx->trace_count = 0;
    
    /* Set default category levels */
    for (int i = 0; i < DEBUG_CAT_MAX; i++) {
        ctx->category_levels[i] = DEBUG_INFO;
    }
    
    return ctx;
}

void debug_context_free(DebugContext *ctx) {
    if (!ctx) return;
    
    if (ctx->log_file && ctx->log_file != stdout && ctx->log_file != stderr) {
        fclose(ctx->log_file);
    }
    
    if (ctx->log_file_path) {
        free(ctx->log_file_path);
    }
    
    free(ctx);
}

void debug_context_set_level(DebugContext *ctx, DebugLevel level) {
    if (!ctx) return;
    ctx->level = level;
}

void debug_context_set_category_level(DebugContext *ctx, DebugCategory category, DebugLevel level) {
    if (!ctx || category >= DEBUG_CAT_MAX) return;
    ctx->category_levels[category] = level;
}

void debug_context_set_output_file(DebugContext *ctx, FILE *file) {
    if (!ctx) return;
    ctx->output_file = file;
}

void debug_context_set_log_file(DebugContext *ctx, const char *path) {
    if (!ctx) return;
    
    if (ctx->log_file && ctx->log_file != stdout && ctx->log_file != stderr) {
        fclose(ctx->log_file);
    }
    
    if (ctx->log_file_path) {
        free(ctx->log_file_path);
    }
    
    ctx->log_file_path = path ? strdup(path) : NULL;
    ctx->log_file = path ? fopen(path, "w") : NULL;
}

void debug_context_set_color(DebugContext *ctx, Bool enabled) {
    if (!ctx) return;
    ctx->color_output = enabled;
}

void debug_context_set_timestamp(DebugContext *ctx, Bool enabled) {
    if (!ctx) return;
    ctx->timestamp_output = enabled;
}

void debug_context_set_show_category(DebugContext *ctx, Bool enabled) {
    if (!ctx) return;
    ctx->show_category = enabled;
}

void debug_context_set_show_location(DebugContext *ctx, Bool enabled) {
    if (!ctx) return;
    ctx->show_location = enabled;
}

/* Utility Functions */
const char* debug_level_to_string(DebugLevel level) {
    switch (level) {
        case DEBUG_NONE: return "NONE";
        case DEBUG_ERROR: return "ERROR";
        case DEBUG_WARNING: return "WARNING";
        case DEBUG_INFO: return "INFO";
        case DEBUG_VERBOSE: return "VERBOSE";
        case DEBUG_TRACE: return "TRACE";
        case DEBUG_ALL: return "ALL";
        default: return "UNKNOWN";
    }
}

const char* debug_category_to_string(DebugCategory category) {
    switch (category) {
        case DEBUG_CAT_GENERAL: return "GENERAL";
        case DEBUG_CAT_LEXER: return "LEXER";
        case DEBUG_CAT_PARSER: return "PARSER";
        case DEBUG_CAT_AST: return "AST";
        case DEBUG_CAT_SYMBOL_TABLE: return "SYMBOL_TABLE";
        case DEBUG_CAT_INTERMEDIATE: return "INTERMEDIATE";
        case DEBUG_CAT_ASSEMBLY: return "ASSEMBLY";
        case DEBUG_CAT_AOT: return "AOT";
        case DEBUG_CAT_MASM: return "MASM";
        case DEBUG_CAT_OPTIMIZATION: return "OPTIMIZATION";
        case DEBUG_CAT_REGISTER: return "REGISTER";
        case DEBUG_CAT_MEMORY: return "MEMORY";
        case DEBUG_CAT_PERFORMANCE: return "PERFORMANCE";
        default: return "UNKNOWN";
    }
}

/* Main Debug Functions */
void debug_log(DebugContext *ctx, DebugLevel level, DebugCategory category, 
               const char *file, int line, const char *function, const char *format, ...) {
    if (!ctx || !format) return;
    
    /* Check if this level should be output */
    if (level > ctx->level || level > ctx->category_levels[category]) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    debug_log_va(ctx, level, category, file, line, function, format, args);
    va_end(args);
}

void debug_log_va(DebugContext *ctx, DebugLevel level, DebugCategory category,
                  const char *file, int line, const char *function, const char *format, va_list args) {
    if (!ctx || !format) return;
    
    /* Update statistics */
    ctx->message_count++;
    switch (level) {
        case DEBUG_ERROR: ctx->error_count++; break;
        case DEBUG_WARNING: ctx->warning_count++; break;
        case DEBUG_INFO: ctx->info_count++; break;
        case DEBUG_VERBOSE: ctx->verbose_count++; break;
        case DEBUG_TRACE: ctx->trace_count++; break;
        default: break;
    }
    
    /* Get current time */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    /* Determine color */
    const char *color = "";
    const char *reset = "";
    if (ctx->color_output) {
        reset = COLOR_RESET;
        switch (level) {
            case DEBUG_ERROR: color = COLOR_RED COLOR_BOLD; break;
            case DEBUG_WARNING: color = COLOR_YELLOW COLOR_BOLD; break;
            case DEBUG_INFO: color = COLOR_GREEN; break;
            case DEBUG_VERBOSE: color = COLOR_CYAN; break;
            case DEBUG_TRACE: color = COLOR_MAGENTA; break;
            default: color = COLOR_WHITE; break;
        }
    }
    
    /* Build prefix */
    char prefix[512] = {0};
    int pos = 0;
    
    if (ctx->timestamp_output) {
        pos += snprintf(prefix + pos, sizeof(prefix) - pos, "[%s] ", timestamp);
    }
    
    if (ctx->show_category) {
        pos += snprintf(prefix + pos, sizeof(prefix) - pos, "[%s] ", debug_category_to_string(category));
    }
    
    pos += snprintf(prefix + pos, sizeof(prefix) - pos, "[%s] ", debug_level_to_string(level));
    
    if (ctx->show_location) {
        pos += snprintf(prefix + pos, sizeof(prefix) - pos, "[%s:%d:%s] ", file, line, function);
    }
    
    /* Output to console */
    if (ctx->output_file) {
        fprintf(ctx->output_file, "%s%s%s", color, prefix, reset);
        vfprintf(ctx->output_file, format, args);
        fprintf(ctx->output_file, "\n");
        fflush(ctx->output_file);
    }
    
    /* Output to log file */
    if (ctx->log_file) {
        fprintf(ctx->log_file, "%s", prefix);
        vfprintf(ctx->log_file, format, args);
        fprintf(ctx->log_file, "\n");
        fflush(ctx->log_file);
    }
}

void debug_print_statistics(DebugContext *ctx) {
    if (!ctx) return;
    
    printf("\n=== Debug Statistics ===\n");
    printf("Total messages: %llu\n", (unsigned long long)ctx->message_count);
    printf("Errors: %llu\n", (unsigned long long)ctx->error_count);
    printf("Warnings: %llu\n", (unsigned long long)ctx->warning_count);
    printf("Info: %llu\n", (unsigned long long)ctx->info_count);
    printf("Verbose: %llu\n", (unsigned long long)ctx->verbose_count);
    printf("Trace: %llu\n", (unsigned long long)ctx->trace_count);
    printf("========================\n");
}

void debug_reset_statistics(DebugContext *ctx) {
    if (!ctx) return;
    
    ctx->message_count = 0;
    ctx->error_count = 0;
    ctx->warning_count = 0;
    ctx->info_count = 0;
    ctx->verbose_count = 0;
    ctx->trace_count = 0;
}

void debug_flush(DebugContext *ctx) {
    if (!ctx) return;
    
    if (ctx->output_file) {
        fflush(ctx->output_file);
    }
    
    if (ctx->log_file) {
        fflush(ctx->log_file);
    }
}

/* Performance Timing */
DebugTimer* debug_timer_start(const char *name) {
    DebugTimer *timer = (DebugTimer*)malloc(sizeof(DebugTimer));
    if (!timer) return NULL;
    
    timer->name = name;
    timer->start_time = clock();
    timer->end_time = 0;
    timer->is_running = true;
    
    return timer;
}

void debug_timer_end(DebugTimer *timer) {
    if (!timer || !timer->is_running) return;
    
    timer->end_time = clock();
    timer->is_running = false;
}

void debug_timer_print(DebugTimer *timer) {
    if (!timer) return;
    
    if (timer->is_running) {
        debug_timer_end(timer);
    }
    
    double elapsed = ((double)(timer->end_time - timer->start_time)) / CLOCKS_PER_SEC;
    DEBUG_PERFORMANCE(DEBUG_INFO, "Timer '%s': %.6f seconds", timer->name, elapsed);
}

void debug_timer_free(DebugTimer *timer) {
    if (timer) {
        free(timer);
    }
}

/* Memory Debugging */
static U64 g_memory_alloc_count = 0;
static U64 g_memory_free_count = 0;
static U64 g_memory_total_allocated = 0;
static U64 g_memory_total_freed = 0;

void debug_memory_alloc(const char *file, int line, const char *function, void *ptr, size_t size) {
    g_memory_alloc_count++;
    g_memory_total_allocated += size;
    
    DEBUG_MEMORY(DEBUG_TRACE, "ALLOC: %p (%zu bytes) at %s:%d:%s", 
                 ptr, size, file, line, function);
}

void debug_memory_free(const char *file, int line, const char *function, void *ptr) {
    g_memory_free_count++;
    
    DEBUG_MEMORY(DEBUG_TRACE, "FREE: %p at %s:%d:%s", 
                 ptr, file, line, function);
}

void debug_memory_print_statistics(void) {
    printf("\n=== Memory Statistics ===\n");
    printf("Allocations: %llu\n", (unsigned long long)g_memory_alloc_count);
    printf("Frees: %llu\n", (unsigned long long)g_memory_free_count);
    printf("Total allocated: %llu bytes\n", (unsigned long long)g_memory_total_allocated);
    printf("Total freed: %llu bytes\n", (unsigned long long)g_memory_total_freed);
    printf("Net allocated: %llu bytes\n", 
           (unsigned long long)(g_memory_total_allocated - g_memory_total_freed));
    printf("=========================\n");
}

/* Command Line Debug Options */
DebugOptions* debug_options_parse(int argc, char *argv[]) {
    DebugOptions *options = (DebugOptions*)malloc(sizeof(DebugOptions));
    if (!options) return NULL;
    
    /* Initialize defaults */
    options->verbose = false;
    options->trace = false;
    options->level = DEBUG_INFO;
    options->log_file = NULL;
    options->color = true;
    options->no_timestamp = false;
    options->show_category = true;
    options->show_location = false;
    options->debug_categories = NULL;
    
    /* Parse arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            options->verbose = true;
            options->level = DEBUG_VERBOSE;
        } else if (strcmp(argv[i], "--trace") == 0) {
            options->trace = true;
            options->level = DEBUG_TRACE;
        } else if (strcmp(argv[i], "--debug-level") == 0 && i + 1 < argc) {
            i++;
            if (strcmp(argv[i], "none") == 0) options->level = DEBUG_NONE;
            else if (strcmp(argv[i], "error") == 0) options->level = DEBUG_ERROR;
            else if (strcmp(argv[i], "warning") == 0) options->level = DEBUG_WARNING;
            else if (strcmp(argv[i], "info") == 0) options->level = DEBUG_INFO;
            else if (strcmp(argv[i], "verbose") == 0) options->level = DEBUG_VERBOSE;
            else if (strcmp(argv[i], "trace") == 0) options->level = DEBUG_TRACE;
            else if (strcmp(argv[i], "all") == 0) options->level = DEBUG_ALL;
        } else if (strcmp(argv[i], "--log-file") == 0 && i + 1 < argc) {
            i++;
            options->log_file = strdup(argv[i]);
        } else if (strcmp(argv[i], "--no-color") == 0) {
            options->color = false;
        } else if (strcmp(argv[i], "--no-timestamp") == 0) {
            options->no_timestamp = true;
        } else if (strcmp(argv[i], "--show-category") == 0) {
            options->show_category = true;
        } else if (strcmp(argv[i], "--show-location") == 0) {
            options->show_location = true;
        } else if (strcmp(argv[i], "--debug-categories") == 0 && i + 1 < argc) {
            i++;
            options->debug_categories = strdup(argv[i]);
        }
    }
    
    return options;
}

void debug_options_apply(DebugContext *ctx, DebugOptions *options) {
    if (!ctx || !options) return;
    
    ctx->level = options->level;
    ctx->color_output = options->color;
    ctx->timestamp_output = !options->no_timestamp;
    ctx->show_category = options->show_category;
    ctx->show_location = options->show_location;
    
    if (options->log_file) {
        debug_context_set_log_file(ctx, options->log_file);
    }
}

void debug_options_free(DebugOptions *options) {
    if (!options) return;
    
    if (options->log_file) {
        free(options->log_file);
    }
    
    if (options->debug_categories) {
        free(options->debug_categories);
    }
    
    free(options);
}

/* Initialization and Cleanup */
void debug_system_init(void) {
    if (g_debug_ctx) return;
    
    g_debug_ctx = debug_context_new();
    if (g_debug_ctx) {
        DEBUG_GENERAL(DEBUG_INFO, "Debug system initialized");
    }
}

void debug_system_cleanup(void) {
    if (g_debug_ctx) {
        DEBUG_GENERAL(DEBUG_INFO, "Debug system shutting down");
        debug_print_statistics(g_debug_ctx);
        debug_memory_print_statistics();
        debug_context_free(g_debug_ctx);
        g_debug_ctx = NULL;
    }
}

/* Enhanced Error Reporting Functions */
void debug_error_with_context(DebugContext *ctx, const char *file __attribute__((unused)), int line __attribute__((unused)), const char *function __attribute__((unused)),
                              const char *source_file, int source_line, int source_column,
                              const char *error_type, const char *message, ...) {
    if (!ctx || ctx->level < DEBUG_ERROR) return;
    
    va_list args;
    va_start(args, message);
    
    /* Print error header */
    if (ctx->color_output) {
        fprintf(ctx->output_file, "%s%sERROR%s: %s%s%s\n", 
                COLOR_RED, COLOR_BOLD, COLOR_RESET,
                COLOR_RED, error_type, COLOR_RESET);
    } else {
        fprintf(ctx->output_file, "ERROR: %s\n", error_type);
    }
    
    /* Print source location */
    if (source_file && source_line > 0) {
        if (ctx->color_output) {
            fprintf(ctx->output_file, "%s  at %s:%d", COLOR_CYAN, source_file, source_line);
            if (source_column > 0) {
                fprintf(ctx->output_file, ":%d", source_column);
            }
            fprintf(ctx->output_file, "%s\n", COLOR_RESET);
        } else {
            fprintf(ctx->output_file, "  at %s:%d", source_file, source_line);
            if (source_column > 0) {
                fprintf(ctx->output_file, ":%d", source_column);
            }
            fprintf(ctx->output_file, "\n");
        }
    }
    
    /* Print error message */
    if (ctx->color_output) {
        fprintf(ctx->output_file, "%s  %s", COLOR_RED, COLOR_RESET);
    } else {
        fprintf(ctx->output_file, "  ");
    }
    vfprintf(ctx->output_file, message, args);
    fprintf(ctx->output_file, "\n");
    
    /* Print source context if available */
    if (source_file && source_line > 0) {
        debug_print_source_context(source_file, source_line, 3);
    }
    
    va_end(args);
    
    /* Update statistics */
    ctx->error_count++;
    ctx->message_count++;
}

void debug_warning_with_context(DebugContext *ctx, const char *file __attribute__((unused)), int line __attribute__((unused)), const char *function __attribute__((unused)),
                                const char *source_file, int source_line, int source_column,
                                const char *warning_type, const char *message, ...) {
    if (!ctx || ctx->level < DEBUG_WARNING) return;
    
    va_list args;
    va_start(args, message);
    
    /* Print warning header */
    if (ctx->color_output) {
        fprintf(ctx->output_file, "%s%sWARNING%s: %s%s%s\n", 
                COLOR_YELLOW, COLOR_BOLD, COLOR_RESET,
                COLOR_YELLOW, warning_type, COLOR_RESET);
    } else {
        fprintf(ctx->output_file, "WARNING: %s\n", warning_type);
    }
    
    /* Print source location */
    if (source_file && source_line > 0) {
        if (ctx->color_output) {
            fprintf(ctx->output_file, "%s  at %s:%d", COLOR_CYAN, source_file, source_line);
            if (source_column > 0) {
                fprintf(ctx->output_file, ":%d", source_column);
            }
            fprintf(ctx->output_file, "%s\n", COLOR_RESET);
        } else {
            fprintf(ctx->output_file, "  at %s:%d", source_file, source_line);
            if (source_column > 0) {
                fprintf(ctx->output_file, ":%d", source_column);
            }
            fprintf(ctx->output_file, "\n");
        }
    }
    
    /* Print warning message */
    if (ctx->color_output) {
        fprintf(ctx->output_file, "%s  %s", COLOR_YELLOW, COLOR_RESET);
    } else {
        fprintf(ctx->output_file, "  ");
    }
    vfprintf(ctx->output_file, message, args);
    fprintf(ctx->output_file, "\n");
    
    /* Print source context if available */
    if (source_file && source_line > 0) {
        debug_print_source_context(source_file, source_line, 2);
    }
    
    va_end(args);
    
    /* Update statistics */
    ctx->warning_count++;
    ctx->message_count++;
}

void debug_print_source_context(const char *source_file, int error_line, int context_lines) {
    FILE *file = fopen(source_file, "r");
    if (!file) return;
    
    char line_buffer[1024];
    int current_line = 1;
    int start_line = (error_line > context_lines) ? error_line - context_lines : 1;
    int end_line = error_line + context_lines;
    
    /* Skip to start line */
    while (current_line < start_line && fgets(line_buffer, sizeof(line_buffer), file)) {
        current_line++;
    }
    
    /* Print context lines */
    while (current_line <= end_line && fgets(line_buffer, sizeof(line_buffer), file)) {
        /* Remove trailing newline */
        size_t len = strlen(line_buffer);
        if (len > 0 && line_buffer[len-1] == '\n') {
            line_buffer[len-1] = '\0';
        }
        
        if (current_line == error_line) {
            /* Highlight error line */
            printf("  %s%4d | %s%s%s\n", COLOR_RED, current_line, COLOR_BOLD, line_buffer, COLOR_RESET);
        } else {
            /* Regular context line */
            printf("  %s%4d | %s%s\n", COLOR_CYAN, current_line, line_buffer, COLOR_RESET);
        }
        
        current_line++;
    }
    
    fclose(file);
}


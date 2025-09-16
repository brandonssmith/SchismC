/*
 * Minimal type definitions for Windows I/O functions
 * Avoids conflicts with Windows headers
 */

#ifndef WINDOWS_IO_TYPES_H
#define WINDOWS_IO_TYPES_H

/* Forward declaration of AOTContext to avoid including full headers */
typedef struct AOTContext AOTContext;

/* Simple boolean type */
typedef int Bool;
#define false 0
#define true 1

/* Function prototypes */
Bool aot_write_binary_windows(AOTContext *ctx, const char *filename);

#endif /* WINDOWS_IO_TYPES_H */



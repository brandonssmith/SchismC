/*
 * Windows API file I/O implementation for AOT binary output
 */

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "aot.h"

Bool aot_write_binary_windows(AOTContext *ctx, const char *filename) {
    if (!ctx || !filename) return false;
    
    printf("DEBUG: aot_write_binary_windows called - ctx: %p, filename: %s\n", (void*)ctx, filename);
    fflush(stdout);
    
    printf("DEBUG: Creating file using Windows API: %s\n", filename);
    fflush(stdout);
    
    /* Validate binary buffer and size */
    if (!ctx->binary_buffer) {
        fputs("ERROR: Binary buffer is NULL\n", stdout);
        fflush(stdout);
        return false;
    }
    
    if (ctx->binary_size <= 0) {
        printf("ERROR: Invalid binary size: %lld\n", ctx->binary_size);
        fflush(stdout);
        return false;
    }
    
    if (ctx->binary_size > 1000000) {  /* Sanity check - 1MB max */
        printf("ERROR: Binary size too large: %lld bytes\n", ctx->binary_size);
        fflush(stdout);
        return false;
    }
    
    printf("DEBUG: Binary buffer validation passed - size: %lld, buffer: %p\n", ctx->binary_size, (void*)ctx->binary_buffer);
    fflush(stdout);
    
    /* Debug: Check if binary size is correct */
    if (ctx->binary_size != 1571) {
        printf("WARNING: Binary size mismatch! Expected 1571, got %lld\n", ctx->binary_size);
        fflush(stdout);
    }
    
    /* Use simple ANSI filename for now */
    printf("DEBUG: Creating file using CreateFileA\n");
    fflush(stdout);
    
    /* Create file using Windows API with ANSI filename */
    HANDLE hFile = CreateFileA(
        filename,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        printf("ERROR: Failed to create file '%s'. Windows error: %lu\n", filename, error);
        fflush(stdout);
        return false;
    }
    
    printf("DEBUG: File created successfully using Windows API\n");
    fflush(stdout);
    
    /* Write binary data using Windows API with proper size handling */
    printf("DEBUG: Writing %lld bytes using WriteFile\n", ctx->binary_size);
    fflush(stdout);
    
    /* Check for size truncation issues */
    if (ctx->binary_size > 0xFFFFFFFF) {  /* 4GB limit for DWORD */
        printf("ERROR: Binary size too large for Windows API: %lld bytes\n", ctx->binary_size);
        CloseHandle(hFile);
        return false;
    }
    
    DWORD bytes_written = 0;
    DWORD size_to_write = (DWORD)ctx->binary_size;
    
    BOOL write_result = WriteFile(
        hFile,
        ctx->binary_buffer,
        size_to_write,
        &bytes_written,
        NULL
    );
    
    if (!write_result) {
        DWORD error = GetLastError();
        printf("ERROR: WriteFile failed. Windows error: %lu\n", error);
        CloseHandle(hFile);
        return false;
    }
    
    /* Verify all bytes were written */
    if (bytes_written != size_to_write) {
        printf("ERROR: WriteFile wrote %lu bytes, expected %lu\n", bytes_written, size_to_write);
        CloseHandle(hFile);
        return false;
    }
    
    printf("DEBUG: Successfully wrote %lu bytes using Windows API\n", bytes_written);
    fflush(stdout);
    
    /* Close file handle */
    if (!CloseHandle(hFile)) {
        DWORD error = GetLastError();
        printf("WARNING: Failed to close file handle. Windows error: %lu\n", error);
        fflush(stdout);
    }
    
    printf("DEBUG: File written successfully using Windows API\n");
    fflush(stdout);
    return true;
}

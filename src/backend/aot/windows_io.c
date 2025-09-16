/*
 * Windows API file I/O implementation for AOT binary output
 */

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

/* Simple types to avoid header conflicts */
typedef int Bool;
#define false 0
#define true 1

/* Forward declaration of AOTContext */
typedef struct {
    unsigned char *binary_buffer;
    long long binary_size;
} AOTContext;

Bool aot_write_binary_windows(AOTContext *ctx, const char *filename) {
    if (!ctx || !filename) return false;
    
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
    
    if (ctx->binary_size > 0xFFFFFFFF) {  /* Sanity check - 4GB max (DWORD limit) */
        printf("ERROR: Binary size too large: %lld bytes (max: 4GB)\n", ctx->binary_size);
        fflush(stdout);
        return false;
    }
    
    printf("DEBUG: Binary buffer validation passed - size: %lld, buffer: %p\n", ctx->binary_size, (void*)ctx->binary_buffer);
    fflush(stdout);
    
    /* Convert filename to wide string for Windows API */
    int filename_len = MultiByteToWideChar(CP_UTF8, 0, filename, -1, NULL, 0);
    if (filename_len == 0) {
        printf("ERROR: Failed to get filename length\n");
        return false;
    }
    
    wchar_t *wide_filename = malloc(filename_len * sizeof(wchar_t));
    if (!wide_filename) {
        printf("ERROR: Failed to allocate memory for wide filename\n");
        return false;
    }
    
    if (MultiByteToWideChar(CP_UTF8, 0, filename, -1, wide_filename, filename_len) == 0) {
        printf("ERROR: Failed to convert filename to wide string\n");
        free(wide_filename);
        return false;
    }
    
    printf("DEBUG: Creating file using CreateFileW\n");
    fflush(stdout);
    
    /* Create file using Windows API */
    HANDLE hFile = CreateFileW(
        wide_filename,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    free(wide_filename);
    
    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        printf("ERROR: Failed to create file '%s'. Windows error: %lu\n", filename, error);
        fflush(stdout);
        return false;
    }
    
    printf("DEBUG: File created successfully using Windows API\n");
    fflush(stdout);
    
    /* Write binary data using Windows API */
    printf("DEBUG: Writing %lld bytes using WriteFile\n", ctx->binary_size);
    fflush(stdout);
    
    DWORD bytes_written = 0;
    BOOL write_result = WriteFile(
        hFile,
        ctx->binary_buffer,
        (DWORD)ctx->binary_size,
        &bytes_written,
        NULL
    );
    
    if (!write_result) {
        DWORD error = GetLastError();
        printf("ERROR: WriteFile failed. Windows error: %lu\n", error);
        CloseHandle(hFile);
        return false;
    }
    
    if (bytes_written != (DWORD)ctx->binary_size) {
        printf("ERROR: WriteFile wrote %lu bytes, expected %lld\n", bytes_written, ctx->binary_size);
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

/*
 * HolyC Runtime Functions Implementation
 * Provides built-in functions for HolyC programs
 */

#include "holyc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>

/*
 * Print function - equivalent to HolyC's Print()
 */
void Print(const char* str, ...) {
    if (!str) return;
    
    va_list args;
    va_start(args, str);
    vprintf(str, args);
    va_end(args);
}

/*
 * Memory allocation functions
 */
void* MAlloc(I64 size) {
    return malloc((size_t)size);
}

void Free(void* ptr) {
    free(ptr);
}

/*
 * String functions
 */
String StrNew(const char* str) {
    if (!str) return NULL;
    
    I64 len = strlen(str);
    String new_str = (String)MAlloc(len + 1);
    if (new_str) {
        strcpy(new_str, str);
    }
    return new_str;
}

String StrPrint(const char* fmt, ...) {
    if (!fmt) return NULL;
    
    va_list args;
    va_start(args, fmt);
    
    /* Get the size needed */
    va_list args_copy;
    va_copy(args_copy, args);
    int size = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);
    
    if (size < 0) {
        va_end(args);
        return NULL;
    }
    
    /* Allocate and format */
    String result = (String)MAlloc(size + 1);
    if (result) {
        vsnprintf(result, size + 1, fmt, args);
    }
    
    va_end(args);
    return result;
}

/*
 * Input functions - HolyC style
 */

/*
 * GetI64 - Get integer input from user
 * Parameters: prompt, default_value, min_value, max_value
 * Returns: I64 value entered by user
 */
I64 GetI64(const char* prompt, I64 default_val, I64 min_val, I64 max_val) {
    I64 value;
    char input_buffer[256];
    
    /* Print prompt if provided */
    if (prompt && strlen(prompt) > 0) {
        printf("%s", prompt);
    }
    
    /* Read input */
    if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
        /* If input fails, return default value */
        return default_val;
    }
    
    /* Parse the input */
    if (sscanf(input_buffer, "%" PRId64, &value) != 1) {
        /* If parsing fails, return default value */
        return default_val;
    }
    
    /* Apply bounds checking */
    if (value < min_val) {
        value = min_val;
    } else if (value > max_val) {
        value = max_val;
    }
    
    return value;
}

/*
 * GetF64 - Get floating point input from user
 * Parameters: prompt, default_value, min_value, max_value
 * Returns: F64 value entered by user
 */
F64 GetF64(const char* prompt, F64 default_val, F64 min_val, F64 max_val) {
    F64 value;
    char input_buffer[256];
    
    /* Print prompt if provided */
    if (prompt && strlen(prompt) > 0) {
        printf("%s", prompt);
    }
    
    /* Read input */
    if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
        /* If input fails, return default value */
        return default_val;
    }
    
    /* Parse the input */
    if (sscanf(input_buffer, "%lf", &value) != 1) {
        /* If parsing fails, return default value */
        return default_val;
    }
    
    /* Apply bounds checking */
    if (value < min_val) {
        value = min_val;
    } else if (value > max_val) {
        value = max_val;
    }
    
    return value;
}

/*
 * GetString - Get string input from user
 * Parameters: prompt, buffer, buffer_size
 * Returns: number of characters read
 */
I64 GetString(const char* prompt, char* buffer, I64 buffer_size) {
    if (!buffer || buffer_size <= 0) return 0;
    
    /* Print prompt if provided */
    if (prompt && strlen(prompt) > 0) {
        printf("%s", prompt);
    }
    
    /* Read input */
    if (fgets(buffer, (int)buffer_size, stdin) == NULL) {
        buffer[0] = '\0';
        return 0;
    }
    
    /* Remove trailing newline */
    I64 len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
        len--;
    }
    
    return len;
}

/*
 * PutChars - Output characters (HolyC style)
 * Parameters: character or string
 */
void PutChars(const char* str) {
    if (str) {
        printf("%s", str);
    }
}

/*
 * PutChar - Output single character
 */
void PutChar(char c) {
    putchar(c);
}

/*
 * Additional utility functions
 */

/*
 * ToI64 - Convert to I64 (HolyC style type casting)
 */
I64 ToI64(F64 value) {
    return (I64)value;
}

/*
 * ToF64 - Convert to F64 (HolyC style type casting)
 */
F64 ToF64(I64 value) {
    return (F64)value;
}

/*
 * ToBool - Convert to Bool (HolyC style type casting)
 */
Bool ToBool(I64 value) {
    return value != 0;
}

/*
 * File I/O Functions - HolyC style
 */

/*
 * FileWrite - Write buffer to file (HolyC style)
 * Parameters: filename, buffer, size, flags
 * Returns: TRUE on success, FALSE on failure
 */
Bool FileWrite(const char* filename, const void* buf, I64 size, I64 flags) {
    if (!filename || !buf || size < 0) {
        return false;
    }
    
    FILE* file = fopen(filename, "wb");
    if (!file) {
        return false;
    }
    
    /* Write data in chunks to handle large files */
    const I64 chunk_size = 0x7FFFFFFF;  /* 2GB chunks */
    const char* buffer_ptr = (const char*)buf;
    I64 remaining_bytes = size;
    I64 total_written = 0;
    
    while (remaining_bytes > 0) {
        I64 bytes_to_write = (remaining_bytes > chunk_size) ? chunk_size : remaining_bytes;
        I64 written = fwrite(buffer_ptr, 1, (size_t)bytes_to_write, file);
        
        if (written != bytes_to_write) {
            fclose(file);
            return false;
        }
        
        total_written += written;
        buffer_ptr += written;
        remaining_bytes -= written;
    }
    
    fclose(file);
    return total_written == size;
}

/*
 * FileRead - Read entire file (HolyC style)
 * Parameters: path, optional size pointer
 * Returns: allocated buffer with file contents, NULL on failure
 */
void* FileRead(const char* path, I64* size) {
    if (!path) return NULL;
    
    FILE* file = fopen(path, "rb");
    if (!file) return NULL;
    
    /* Get file size */
    fseek(file, 0, SEEK_END);
    I64 file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size < 0) {
        fclose(file);
        return NULL;
    }
    
    /* Allocate buffer */
    void* buffer = MAlloc(file_size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    
    /* Read file */
    I64 bytes_read = fread(buffer, 1, (size_t)file_size, file);
    fclose(file);
    
    if (bytes_read != file_size) {
        Free(buffer);
        return NULL;
    }
    
    /* Null terminate for safety */
    ((char*)buffer)[file_size] = '\0';
    
    if (size) {
        *size = file_size;
    }
    
    return buffer;
}
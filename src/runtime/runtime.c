#include "holyc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// Runtime functions for HolyC

void Print(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void PutChars(const char* str) {
    if (str) {
        printf("%s", str);
    }
}

I64 StrLen(const char* str) {
    if (!str) return 0;
    return (I64)strlen(str);
}

String StrNew(const char* str) {
    if (!str) return NULL;
    I64 len = strlen(str) + 1;
    String new_str = MAlloc(len);
    strcpy(new_str, str);
    return new_str;
}

String StrPrint(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    // Get the size needed
    va_list args_copy;
    va_copy(args_copy, args);
    int size = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);
    
    if (size < 0) {
        va_end(args);
        return NULL;
    }
    
    // Allocate and format
    String str = MAlloc(size + 1);
    vsnprintf(str, size + 1, format, args);
    va_end(args);
    
    return str;
}

String StrPrintJoin(String existing, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    // Get the size needed
    va_list args_copy;
    va_copy(args_copy, args);
    int size = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);
    
    if (size < 0) {
        va_end(args);
        return existing;
    }
    
    // Allocate and format
    String str = MAlloc(size + 1);
    vsnprintf(str, size + 1, format, args);
    va_end(args);
    
    if (existing) {
        Free(existing);
    }
    
    return str;
}

void Free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

void* MAlloc(I64 size) {
    if (size <= 0) return NULL;
    return malloc((size_t)size);
}

void* CAlloc(I64 size) {
    if (size <= 0) return NULL;
    void* ptr = malloc((size_t)size);
    if (ptr) {
        memset(ptr, 0, (size_t)size);
    }
    return ptr;
}

void* ReAlloc(void* ptr, I64 size) {
    if (size <= 0) {
        Free(ptr);
        return NULL;
    }
    return realloc(ptr, (size_t)size);
}

I64 MSize(void* ptr) {
    // This is a simplified version - real implementation would track sizes
    (void)ptr;
    return 0;
}

// String manipulation functions
String StrCpy(String dest, const char* src) {
    if (!dest || !src) return dest;
    strcpy(dest, src);
    return dest;
}

String StrCat(String dest, const char* src) {
    if (!dest || !src) return dest;
    strcat(dest, src);
    return dest;
}

I64 StrCmp(const char* str1, const char* str2) {
    if (!str1 || !str2) return str1 - str2;
    return strcmp(str1, str2);
}

String StrChr(const char* str, int ch) {
    if (!str) return NULL;
    return (String)strchr(str, ch);
}

String StrRChr(const char* str, int ch) {
    if (!str) return NULL;
    return (String)strrchr(str, ch);
}

String StrStr(const char* haystack, const char* needle) {
    if (!haystack || !needle) return NULL;
    return (String)strstr(haystack, needle);
}

// Memory functions
void MemSet(void* ptr, int value, I64 size) {
    if (ptr && size > 0) {
        memset(ptr, value, (size_t)size);
    }
}

void MemCpy(void* dest, const void* src, I64 size) {
    if (dest && src && size > 0) {
        memcpy(dest, src, (size_t)size);
    }
}

void MemMove(void* dest, const void* src, I64 size) {
    if (dest && src && size > 0) {
        memmove(dest, src, (size_t)size);
    }
}

I64 MemCmp(const void* ptr1, const void* ptr2, I64 size) {
    if (!ptr1 || !ptr2 || size <= 0) return 0;
    return memcmp(ptr1, ptr2, (size_t)size);
}

// Math functions
F64 Pow10I64(I64 exp) {
    F64 result = 1.0;
    if (exp >= 0) {
        for (I64 i = 0; i < exp; i++) {
            result *= 10.0;
        }
    } else {
        for (I64 i = 0; i < -exp; i++) {
            result /= 10.0;
        }
    }
    return result;
}

I64 AbsI64(I64 value) {
    return value < 0 ? -value : value;
}

F64 AbsF64(F64 value) {
    return value < 0.0 ? -value : value;
}

I64 MinI64(I64 a, I64 b) {
    return a < b ? a : b;
}

I64 MaxI64(I64 a, I64 b) {
    return a > b ? a : b;
}

F64 MinF64(F64 a, F64 b) {
    return a < b ? a : b;
}

F64 MaxF64(F64 a, F64 b) {
    return a > b ? a : b;
}

// Type conversion functions
I64 ToI64(F64 value) {
    return (I64)value;
}

F64 ToF64(I64 value) {
    return (F64)value;
}

Bool ToBool(I64 value) {
    return value != 0;
}

// Character functions
int ToUpper(int ch) {
    if (ch >= 'a' && ch <= 'z') {
        return ch - 'a' + 'A';
    }
    return ch;
}

int ToLower(int ch) {
    if (ch >= 'A' && ch <= 'Z') {
        return ch - 'A' + 'a';
    }
    return ch;
}

Bool IsAlpha(int ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

Bool IsDigit(int ch) {
    return ch >= '0' && ch <= '9';
}

Bool IsAlNum(int ch) {
    return IsAlpha(ch) || IsDigit(ch);
}

// File functions
FILE* FileOpen(const char* filename, const char* mode) {
    return fopen(filename, mode);
}

I64 FileRead(const char* filename, void* buffer, I64 size) {
    FILE* file = fopen(filename, "rb");
    if (!file) return -1;
    
    I64 bytes_read = fread(buffer, 1, (size_t)size, file);
    fclose(file);
    return bytes_read;
}

I64 FileWrite(const char* filename, const void* buffer, I64 size) {
    FILE* file = fopen(filename, "wb");
    if (!file) return -1;
    
    I64 bytes_written = fwrite(buffer, 1, (size_t)size, file);
    fclose(file);
    return bytes_written;
}

// Error handling
void Error(const char* message, ...) {
    va_list args;
    va_start(args, message);
    fprintf(stderr, "Error: ");
    vfprintf(stderr, message, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(1);
}

void Warning(const char* message, ...) {
    va_list args;
    va_start(args, message);
    fprintf(stderr, "Warning: ");
    vfprintf(stderr, message, args);
    fprintf(stderr, "\n");
    va_end(args);
}

/*
 * AOT (Ahead-of-Time) Compilation Implementation
 * Assembly symbol resolution and native executable generation for SchismC
 * Based on HolyC's AsmResolve.HC and CmpJoin() functionality
 */

#include "aot.h"
#include <string.h>
#include <stdlib.h>

/*
 * AOT Context Management
 */

AOTContext* aot_context_new(CCmpCtrl *cc, AssemblyContext *asm_ctx) {
    AOTContext *ctx = malloc(sizeof(AOTContext));
    if (!ctx) return NULL;
    
    memset(ctx, 0, sizeof(AOTContext));
    ctx->cc = cc;
    ctx->asm_ctx = asm_ctx;
    
    /* Initialize AOT state */
    ctx->aot = aot_new();
    if (!ctx->aot) {
        free(ctx);
        return NULL;
    }
    
    ctx->aotc = malloc(sizeof(CAOTCtrl));
    if (!ctx->aotc) {
        aot_free(ctx->aot);
        free(ctx);
        return NULL;
    }
    memset(ctx->aotc, 0, sizeof(CAOTCtrl));
    
    ctx->aot_depth = 0;
    ctx->num_unresolved = 0;
    ctx->num_imports = 0;
    ctx->num_exports = 0;
    
    /* Initialize PE sections */
    ctx->num_sections = 3;  /* .text, .data, .rdata */
    ctx->pe_sections = malloc(ctx->num_sections * sizeof(PESectionHeader));
    if (!ctx->pe_sections) {
        free(ctx->aotc);
        aot_free(ctx->aot);
        free(ctx);
        return NULL;
    }
    
    /* Initialize binary buffer */
    ctx->binary_capacity = 65536;  /* Start with 64KB */
    ctx->binary_buffer = malloc(ctx->binary_capacity);
    if (!ctx->binary_buffer) {
        free(ctx->pe_sections);
        free(ctx->aotc);
        aot_free(ctx->aot);
        free(ctx);
        return NULL;
    }
    
    ctx->binary_size = 0;
    
    return ctx;
}

void aot_context_free(AOTContext *ctx) {
    if (!ctx) return;
    
    if (ctx->binary_buffer) free(ctx->binary_buffer);
    if (ctx->pe_sections) free(ctx->pe_sections);
    if (ctx->aotc) free(ctx->aotc);
    if (ctx->aot) aot_free(ctx->aot);
    
    free(ctx);
}

/*
 * Assembly Symbol Resolution
 */

Bool aot_resolve_symbols(AOTContext *ctx) {
    if (!ctx) return false;
    
    /* Resolve imports first */
    if (!aot_resolve_imports(ctx)) return false;
    
    /* Resolve exports */
    if (!aot_resolve_exports(ctx)) return false;
    
    /* Resolve relocations */
    if (!aot_resolve_relocations(ctx)) return false;
    
    return true;
}

Bool aot_resolve_imports(AOTContext *ctx) {
    if (!ctx) return true;  /* No imports to resolve yet */
    
    /* TODO: Implement import resolution */
    /* This would resolve Windows API calls and external symbols */
    
    return true;
}

Bool aot_resolve_exports(AOTContext *ctx) {
    if (!ctx) return true;  /* No exports to resolve yet */
    
    /* TODO: Implement export resolution */
    /* This would resolve exported symbols for linking */
    
    return true;
}

Bool aot_resolve_relocations(AOTContext *ctx) {
    if (!ctx) return true;
    
    /* Process unresolved references */
    CAsmUnresolvedRef *ref = ctx->unresolved_refs;
    while (ref) {
        I64 address;
        if (aot_resolve_symbol(ctx, ref->str, &address)) {
            /* Apply relocation */
            U8 *ptr = ctx->binary_buffer + ref->rip;
            switch (ref->type) {
                case IET_REL_I8:
                    *(I8*)ptr = (I8)(address - ref->rip - 1);
                    break;
                case IET_REL_I16:
                    *(I16*)ptr = (I16)(address - ref->rip - 2);
                    break;
                case IET_REL_I32:
                    *(I32*)ptr = (I32)(address - ref->rip - 4);
                    break;
                case IET_REL_I64:
                    *(I64*)ptr = address - ref->rip - 8;
                    break;
                case IET_IMM_U8:
                    *(U8*)ptr = (U8)address;
                    break;
                case IET_IMM_U16:
                    *(U16*)ptr = (U16)address;
                    break;
                case IET_IMM_U32:
                    *(U32*)ptr = (U32)address;
                    break;
                case IET_IMM_U64:
                    *(U64*)ptr = address;
                    break;
            }
        }
        
        ref = ref->next;
    }
    
    return true;
}

Bool aot_resolve_symbol(AOTContext *ctx, const char *symbol_name, I64 *address) {
    if (!ctx || !symbol_name || !address) return false;
    
    /* TODO: Implement symbol resolution */
    /* This would look up symbols in hash tables, resolve Windows API calls, etc. */
    
    *address = 0;  /* Placeholder */
    return false;
}

/*
 * PE Format Generation
 */

Bool aot_generate_pe_header(AOTContext *ctx) {
    if (!ctx) return false;
    
    printf("DEBUG: Generating PE headers for Windows executable\n");
    
    /* Generate DOS stub */
    U8 dos_stub[] = {
        0x4D, 0x5A, 0x90, 0x00, 0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0x00, 0x00, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x80, 0x00, 0x00, 0x00, 0x0E, 0x1F, 0xBA, 0x0E, 0x00, 0xB4, 0x09, 0xCD,
        0x21, 0xB8, 0x01, 0x4C, 0xCD, 0x21, 0x54, 0x68, 0x69, 0x73, 0x20, 0x70,
        0x72, 0x6F, 0x67, 0x72, 0x61, 0x6D, 0x20, 0x63, 0x61, 0x6E, 0x6E, 0x6F,
        0x74, 0x20, 0x62, 0x65, 0x20, 0x72, 0x75, 0x6E, 0x20, 0x69, 0x6E, 0x20,
        0x44, 0x4F, 0x53, 0x20, 0x6D, 0x6F, 0x64, 0x65, 0x2E, 0x0D, 0x0D, 0x0A,
        0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    
    /* Append DOS stub to binary */
    if (!aot_append_binary(ctx, dos_stub, sizeof(dos_stub))) {
        return false;
    }
    
    /* Generate PE COFF header */
    memset(&ctx->pe_header, 0, sizeof(PECOFFHeader));
    ctx->pe_header.machine = 0x014C;  /* x86 (32-bit) for testing */
    ctx->pe_header.num_sections = ctx->num_sections;
    ctx->pe_header.time_stamp = 0;  /* TODO: Get current timestamp */
    ctx->pe_header.size_of_optional_header = sizeof(PEOptionalHeader);
    ctx->pe_header.characteristics = 0x02;  /* EXECUTABLE_IMAGE (x64) */
    
    /* Generate PE optional header */
    memset(&ctx->pe_optional, 0, sizeof(PEOptionalHeader));
    ctx->pe_optional.magic = 0x010B;  /* PE32 (32-bit) */
    ctx->pe_optional.major_linker_version = 14;  /* Visual Studio 2015+ */
    ctx->pe_optional.minor_linker_version = 0;
    ctx->pe_optional.size_of_code = 0;  /* Will be calculated */
    ctx->pe_optional.size_of_initialized_data = 0;  /* Will be calculated */
    ctx->pe_optional.size_of_uninitialized_data = 0;
    ctx->pe_optional.address_of_entry_point = 0x1000;  /* Entry point in .text section */
    ctx->pe_optional.base_of_code = 0x1000;  /* Base of code section */
    ctx->pe_optional.image_base = 0x400000;  /* Default image base for x86 */
    ctx->pe_optional.section_alignment = 0x1000;  /* 4KB alignment */
    ctx->pe_optional.file_alignment = 0x200;  /* 512 byte alignment */
    ctx->pe_optional.major_os_version = 6;  /* Windows Vista+ */
    ctx->pe_optional.minor_os_version = 0;
    ctx->pe_optional.major_image_version = 1;
    ctx->pe_optional.minor_image_version = 0;
    ctx->pe_optional.major_subsystem_version = 6;
    ctx->pe_optional.minor_subsystem_version = 0;
    ctx->pe_optional.win32_version_value = 0;
    ctx->pe_optional.size_of_image = 0;  /* Will be calculated */
    ctx->pe_optional.size_of_headers = 0;  /* Will be calculated */
    ctx->pe_optional.checksum = 0;  /* Will be calculated */
    ctx->pe_optional.subsystem = PE_SUBSYSTEM_CONSOLE;
    ctx->pe_optional.dll_characteristics = 0;
    ctx->pe_optional.size_of_stack_reserve = 0x100000;  /* 1MB stack */
    ctx->pe_optional.size_of_stack_commit = 0x4000;  /* 16KB initial stack */
    ctx->pe_optional.size_of_heap_reserve = 0x1000000;  /* 16MB heap */
    ctx->pe_optional.size_of_heap_commit = 0x10000;  /* 64KB initial heap */
    ctx->pe_optional.loader_flags = 0;
    ctx->pe_optional.num_rva_and_sizes = 16;  /* Standard number of data directories */
    
    /* Append PE signature to binary */
    U32 pe_signature = 0x00004550;  /* "PE\0\0" */
    if (!aot_append_binary(ctx, (U8*)&pe_signature, sizeof(U32))) {
        return false;
    }
    
    /* Append COFF header to binary */
    if (!aot_append_binary(ctx, (U8*)&ctx->pe_header, sizeof(PECOFFHeader))) {
        return false;
    }
    
    /* Append optional header to binary */
    if (!aot_append_binary(ctx, (U8*)&ctx->pe_optional, sizeof(PEOptionalHeader))) {
        return false;
    }
    
    /* Append data directories (all zeros for now) */
    U8 data_directories[16 * 8] = {0};  /* 16 entries, 8 bytes each */
    if (!aot_append_binary(ctx, data_directories, sizeof(data_directories))) {
        return false;
    }
    
    return true;
}

Bool aot_generate_sections(AOTContext *ctx) {
    if (!ctx || !ctx->pe_sections) return false;
    
    /* .text section (code) */
    memset(&ctx->pe_sections[0], 0, sizeof(PESectionHeader));
    strncpy((char*)ctx->pe_sections[0].name, ".text", 8);
    ctx->pe_sections[0].virtual_size = 0;  /* Will be calculated */
    ctx->pe_sections[0].virtual_address = 0x1000;
    ctx->pe_sections[0].size_of_raw_data = 0;  /* Will be calculated */
    ctx->pe_sections[0].ptr_to_raw_data = 0;  /* Will be calculated */
    ctx->pe_sections[0].characteristics = 0x60000020;  /* CODE | EXECUTE | READ */
    
    /* .data section (initialized data) */
    memset(&ctx->pe_sections[1], 0, sizeof(PESectionHeader));
    strncpy((char*)ctx->pe_sections[1].name, ".data", 8);
    ctx->pe_sections[1].virtual_size = 0;  /* Will be calculated */
    ctx->pe_sections[1].virtual_address = 0x2000;
    ctx->pe_sections[1].size_of_raw_data = 0;  /* Will be calculated */
    ctx->pe_sections[1].ptr_to_raw_data = 0;  /* Will be calculated */
    ctx->pe_sections[1].characteristics = 0xC0000040;  /* INITIALIZED_DATA | READ | WRITE */
    
    /* .rdata section (read-only data) */
    memset(&ctx->pe_sections[2], 0, sizeof(PESectionHeader));
    strncpy((char*)ctx->pe_sections[2].name, ".rdata", 8);
    ctx->pe_sections[2].virtual_size = 0;  /* Will be calculated */
    ctx->pe_sections[2].virtual_address = 0x3000;
    ctx->pe_sections[2].size_of_raw_data = 0;  /* Will be calculated */
    ctx->pe_sections[2].ptr_to_raw_data = 0;  /* Will be calculated */
    ctx->pe_sections[2].characteristics = 0x40000040;  /* INITIALIZED_DATA | READ */
    
    /* Add .idata section for import table */
    ctx->num_sections = 4;
    ctx->pe_sections = realloc(ctx->pe_sections, ctx->num_sections * sizeof(PESectionHeader));
    if (!ctx->pe_sections) return false;
    
    /* .idata section (import table) */
    memset(&ctx->pe_sections[3], 0, sizeof(PESectionHeader));
    strncpy((char*)ctx->pe_sections[3].name, ".idata", 8);
    ctx->pe_sections[3].virtual_size = 0;  /* Will be calculated */
    ctx->pe_sections[3].virtual_address = 0x4000;
    ctx->pe_sections[3].size_of_raw_data = 0;  /* Will be calculated */
    ctx->pe_sections[3].ptr_to_raw_data = 0;  /* Will be calculated */
    ctx->pe_sections[3].characteristics = 0x40000040;  /* INITIALIZED_DATA | READ */
    
    printf("DEBUG: Generated %lld PE sections (.text, .data, .rdata, .idata)\n", ctx->num_sections);
    
    /* Append section headers to binary */
    if (!aot_append_binary(ctx, (U8*)ctx->pe_sections, ctx->num_sections * sizeof(PESectionHeader))) {
        return false;
    }
    
    return true;
}

Bool aot_generate_import_table(AOTContext *ctx) {
    if (!ctx) return false;
    
    printf("DEBUG: Generating Windows API import table\n");
    
    /* Add basic Windows API imports for console applications */
    const char *kernel32_apis[] = {
        "ExitProcess",
        "GetStdHandle", 
        "WriteConsoleA",
        "ReadConsoleA"
    };
    
    const char *msvcrt_apis[] = {
        "printf",
        "puts",
        "scanf",
        "malloc",
        "free"
    };
    
    /* Add imports to context */
    for (int i = 0; i < sizeof(kernel32_apis) / sizeof(kernel32_apis[0]); i++) {
        if (!aot_add_import(ctx, kernel32_apis[i], IET_IMPORT_U64, 0)) {
            printf("WARNING: Failed to add import: %s\n", kernel32_apis[i]);
        }
    }
    
    for (int i = 0; i < sizeof(msvcrt_apis) / sizeof(msvcrt_apis[0]); i++) {
        if (!aot_add_import(ctx, msvcrt_apis[i], IET_IMPORT_U64, 0)) {
            printf("WARNING: Failed to add import: %s\n", msvcrt_apis[i]);
        }
    }
    
    /* Generate import descriptor table */
    if (!aot_generate_import_descriptor_table(ctx)) {
        return false;
    }
    
    return true;
}

Bool aot_generate_export_table(AOTContext *ctx) {
    if (!ctx) return true;  /* No exports yet */
    
    /* TODO: Generate export table for exported symbols */
    
    return true;
}

Bool aot_generate_relocations(AOTContext *ctx) {
    if (!ctx) return true;
    
    /* TODO: Generate relocation table */
    
    return true;
}

/*
 * Binary Output
 */

Bool aot_write_binary(AOTContext *ctx, const char *filename) {
    if (!ctx || !filename) return false;
    
    FILE *file = fopen(filename, "wb");
    if (!file) return false;
    
    /* Write the entire binary buffer (includes DOS stub, PE headers, and data) */
    fwrite(ctx->binary_buffer, 1, ctx->binary_size, file);
    
    fclose(file);
    return true;
}

Bool aot_append_binary(AOTContext *ctx, const U8 *data, I64 size) {
    if (!ctx || !data || size <= 0) return false;
    
    /* Check if we need to expand buffer */
    if (ctx->binary_size + size > ctx->binary_capacity) {
        I64 new_capacity = ctx->binary_capacity * 2;
        while (new_capacity < ctx->binary_size + size) {
            new_capacity *= 2;
        }
        
        U8 *new_buffer = realloc(ctx->binary_buffer, new_capacity);
        if (!new_buffer) return false;
        
        ctx->binary_buffer = new_buffer;
        ctx->binary_capacity = new_capacity;
    }
    
    /* Append data */
    memcpy(ctx->binary_buffer + ctx->binary_size, data, size);
    ctx->binary_size += size;
    
    return true;
}

Bool aot_align_binary(AOTContext *ctx, I64 alignment) {
    if (!ctx || alignment <= 0) return false;
    
    I64 padding = (alignment - (ctx->binary_size % alignment)) % alignment;
    if (padding > 0) {
        /* Add padding bytes */
        U8 *padding_bytes = malloc(padding);
        if (!padding_bytes) return false;
        
        memset(padding_bytes, 0, padding);
        Bool result = aot_append_binary(ctx, padding_bytes, padding);
        free(padding_bytes);
        
        return result;
    }
    
    return true;
}

/*
 * Main AOT Compilation Function (CmpJoin equivalent)
 */

CAOT* aot_compile_join(AOTContext *ctx, I64 cmp_flags, const char *map_name) {
    if (!ctx) return NULL;
    
    /* Initialize AOT compilation */
    ctx->aot_depth++;
    
    /* Generate PE headers */
    if (!aot_generate_pe_header(ctx)) return NULL;
    if (!aot_generate_sections(ctx)) return NULL;
    
    /* Generate Windows entry point */
    if (!aot_generate_windows_entry_point(ctx)) {
        printf("ERROR: Failed to generate Windows entry point\n");
        return NULL;
    }
    
    /* Generate assembly code */
    I64 assembly_size;
    U8 *assembly = assembly_generate_code(ctx->asm_ctx, &assembly_size);
    if (!assembly) return NULL;
    
    /* Append assembly to binary */
    if (!aot_append_binary(ctx, assembly, assembly_size)) {
        free(assembly);
        return NULL;
    }
    
    /* Resolve symbols */
    if (!aot_resolve_symbols(ctx)) {
        free(assembly);
        return NULL;
    }
    
    /* Generate import/export tables */
    if (!aot_generate_import_table(ctx)) {
        free(assembly);
        return NULL;
    }
    if (!aot_generate_export_table(ctx)) {
        free(assembly);
        return NULL;
    }
    
    /* Generate relocations */
    if (!aot_generate_relocations(ctx)) {
        free(assembly);
        return NULL;
    }
    
    /* Update PE headers with actual sizes */
    ctx->pe_sections[0].size_of_raw_data = assembly_size;
    ctx->pe_optional.size_of_code = assembly_size;
    ctx->pe_optional.size_of_image = ctx->binary_size + 0x1000;  /* Add header space */
    
    free(assembly);
    return ctx->aot;
}

/*
 * AOT Compilation to Executable
 */

Bool aot_compile_to_executable(AOTContext *ctx, const char *output_filename) {
    if (!ctx || !output_filename) return false;
    
    /* Compile to AOT */
    CAOT *aot = aot_compile_join(ctx, 0, NULL);
    if (!aot) return false;
    
    /* Write binary to file */
    return aot_write_binary(ctx, output_filename);
}

/*
 * Utility Functions
 */

I64 aot_calculate_checksum(U8 *data, I64 size) {
    if (!data || size <= 0) return 0;
    
    U32 checksum = 0;
    for (I64 i = 0; i < size; i += 2) {
        if (i + 1 < size) {
            checksum += (U32)(data[i] | (data[i + 1] << 8));
        } else {
            checksum += (U32)data[i];
        }
    }
    
    return (I64)checksum;
}

Bool aot_validate_pe_format(AOTContext *ctx) {
    if (!ctx) return false;
    
    /* Validate PE headers */
    if (ctx->pe_header.machine != PE_MACHINE_X64) return false;
    if (ctx->pe_header.num_sections != ctx->num_sections) return false;
    if (ctx->pe_optional.magic != 0x20b) return false;
    
    return true;
}

void aot_print_debug_info(AOTContext *ctx) {
    if (!ctx) return;
    
    printf("AOT Debug Information:\n");
    printf("  Binary size: %lld bytes\n", ctx->binary_size);
    printf("  Number of sections: %lld\n", ctx->num_sections);
    printf("  Number of imports: %lld\n", ctx->num_imports);
    printf("  Number of exports: %lld\n", ctx->num_exports);
    printf("  Number of unresolved: %lld\n", ctx->num_unresolved);
    printf("  PE machine: 0x%04X\n", ctx->pe_header.machine);
    printf("  PE subsystem: %d\n", ctx->pe_optional.subsystem);
}

/*
 * Windows API Integration Functions
 */

Bool aot_add_import(AOTContext *ctx, const char *symbol_name, ImportExportType type, I64 address) {
    if (!ctx || !symbol_name) return false;
    
    /* Expand imports array if needed */
    if (!ctx->imports || ctx->num_imports == 0) {
        ctx->imports = malloc(10 * sizeof(CAOTImportExport));
        if (!ctx->imports) return false;
    }
    
    /* Add import entry */
    CAOTImportExport *import = &ctx->imports[ctx->num_imports];
    memset(import, 0, sizeof(CAOTImportExport));
    
    /* Allocate and copy symbol name */
    import->str = malloc(strlen(symbol_name) + 1);
    if (!import->str) return false;
    strcpy((char*)import->str, symbol_name);
    
    import->type = type;
    import->rip = address;
    import->flags = 0;
    
    ctx->num_imports++;
    
    printf("DEBUG: Added import: %s (type: %d, address: 0x%llx)\n", 
           symbol_name, type, address);
    
    return true;
}

Bool aot_generate_import_descriptor_table(AOTContext *ctx) {
    if (!ctx) return false;
    
    printf("DEBUG: Generating complete import descriptor table\n");
    
    /* Calculate RVAs for import structures */
    U32 current_rva = 0x5000;  /* Start of import section */
    
    /* Import Directory Table (IDT) */
    U32 idt_rva = current_rva;
    current_rva += 60;  /* 3 descriptors * 20 bytes */
    
    /* Import Lookup Tables (ILT) */
    U32 kernel32_ilt_rva = current_rva;
    current_rva += 32;  /* 4 functions * 8 bytes */
    
    U32 msvcrt_ilt_rva = current_rva;
    current_rva += 40;  /* 5 functions * 8 bytes */
    
    /* Import Address Tables (IAT) */
    U32 kernel32_iat_rva = current_rva;
    current_rva += 32;  /* 4 functions * 8 bytes */
    
    U32 msvcrt_iat_rva = current_rva;
    current_rva += 40;  /* 5 functions * 8 bytes */
    
    /* Import Name Tables (INT) */
    U32 kernel32_int_rva = current_rva;
    current_rva += 32;  /* 4 functions * 8 bytes */
    
    U32 msvcrt_int_rva = current_rva;
    current_rva += 40;  /* 5 functions * 8 bytes */
    
    /* DLL Names */
    U32 kernel32_name_rva = current_rva;
    current_rva += 13;  /* "KERNEL32.dll" + null */
    
    U32 msvcrt_name_rva = current_rva;
    current_rva += 10;  /* "msvcrt.dll" + null */
    
    /* Function Names */
    U32 function_names_rva = current_rva;
    
    /* Generate import directory table for kernel32.dll */
    struct {
        U32 import_lookup_table_rva;
        U32 time_date_stamp;
        U32 forwarder_chain;
        U32 name_rva;
        U32 import_address_table_rva;
    } kernel32_descriptor = {
        kernel32_ilt_rva,
        0,       /* Time date stamp */
        0,       /* Forwarder chain */
        kernel32_name_rva,
        kernel32_iat_rva
    };
    
    /* Generate import directory table for msvcrt.dll */
    struct {
        U32 import_lookup_table_rva;
        U32 time_date_stamp;
        U32 forwarder_chain;
        U32 name_rva;
        U32 import_address_table_rva;
    } msvcrt_descriptor = {
        msvcrt_ilt_rva,
        0,       /* Time date stamp */
        0,       /* Forwarder chain */
        msvcrt_name_rva,
        msvcrt_iat_rva
    };
    
    /* Null terminator */
    struct {
        U32 import_lookup_table_rva;
        U32 time_date_stamp;
        U32 forwarder_chain;
        U32 name_rva;
        U32 import_address_table_rva;
    } null_descriptor = {0};
    
    /* Write import descriptors */
    if (!aot_append_binary(ctx, (U8*)&kernel32_descriptor, sizeof(kernel32_descriptor))) {
        return false;
    }
    if (!aot_append_binary(ctx, (U8*)&msvcrt_descriptor, sizeof(msvcrt_descriptor))) {
        return false;
    }
    if (!aot_append_binary(ctx, (U8*)&null_descriptor, sizeof(null_descriptor))) {
        return false;
    }
    
    /* Generate Import Lookup Tables */
    U64 kernel32_functions[] = {
        function_names_rva,      /* ExitProcess */
        function_names_rva + 13, /* GetStdHandle */
        function_names_rva + 26, /* WriteConsoleA */
        function_names_rva + 39, /* ReadConsoleA */
        0                        /* Null terminator */
    };
    
    U64 msvcrt_functions[] = {
        function_names_rva + 52, /* printf */
        function_names_rva + 59, /* puts */
        function_names_rva + 64, /* scanf */
        function_names_rva + 71, /* malloc */
        function_names_rva + 78, /* free */
        0                        /* Null terminator */
    };
    
    /* Write Import Lookup Tables */
    if (!aot_append_binary(ctx, (U8*)kernel32_functions, sizeof(kernel32_functions))) {
        return false;
    }
    if (!aot_append_binary(ctx, (U8*)msvcrt_functions, sizeof(msvcrt_functions))) {
        return false;
    }
    
    /* Write Import Address Tables (initially same as ILT) */
    if (!aot_append_binary(ctx, (U8*)kernel32_functions, sizeof(kernel32_functions))) {
        return false;
    }
    if (!aot_append_binary(ctx, (U8*)msvcrt_functions, sizeof(msvcrt_functions))) {
        return false;
    }
    
    /* Write Import Name Tables (same as ILT for now) */
    if (!aot_append_binary(ctx, (U8*)kernel32_functions, sizeof(kernel32_functions))) {
        return false;
    }
    if (!aot_append_binary(ctx, (U8*)msvcrt_functions, sizeof(msvcrt_functions))) {
        return false;
    }
    
    /* Generate DLL names */
    const char *kernel32_name = "KERNEL32.dll";
    const char *msvcrt_name = "msvcrt.dll";
    
    if (!aot_append_binary(ctx, (U8*)kernel32_name, strlen(kernel32_name) + 1)) {
        return false;
    }
    if (!aot_append_binary(ctx, (U8*)msvcrt_name, strlen(msvcrt_name) + 1)) {
        return false;
    }
    
    /* Generate function names with hints */
    struct {
        U16 hint;
        char name[16];
    } function_hints[] = {
        {0, "ExitProcess\0"},
        {0, "GetStdHandle\0"},
        {0, "WriteConsoleA\0"},
        {0, "ReadConsoleA\0"},
        {0, "printf\0"},
        {0, "puts\0"},
        {0, "scanf\0"},
        {0, "malloc\0"},
        {0, "free\0"}
    };
    
    if (!aot_append_binary(ctx, (U8*)function_hints, sizeof(function_hints))) {
        return false;
    }
    
    printf("DEBUG: Complete import descriptor table generated successfully\n");
    
    return true;
}

Bool aot_generate_windows_entry_point(AOTContext *ctx) {
    if (!ctx) return false;
    
    printf("DEBUG: Generating Windows entry point with API calls\n");
    
    /* Generate entry point assembly code:
     * 
     * main:
     *   sub rsp, 32          ; Stack alignment for Windows calling convention
     *   call user_main       ; Call the user's main function
     *   mov rcx, rax         ; Set exit code
     *   call ExitProcess     ; Exit with return code
     *   add rsp, 32          ; Restore stack
     *   ret
     * 
     * user_main:
     *   ; User code will be inserted here
     *   mov rax, 0           ; Return 0
     *   ret
     */
    
    U8 entry_point_code[] = {
        /* main: */
        0x83, 0xEC, 0x20,        /* sub esp, 32 */
        0xE8, 0x00, 0x00, 0x00, 0x00,  /* call user_main (will be patched) */
        0x50,                    /* push eax (exit code) */
        0xFF, 0x15, 0x00, 0x00, 0x00, 0x00,  /* call [ExitProcess] (will be patched) */
        0x83, 0xC4, 0x24,        /* add esp, 36 (32 + 4 for pushed eax) */
        0xC3,                    /* ret */
        
        /* user_main: */
        0xB8, 0x00, 0x00, 0x00, 0x00,  /* mov eax, 0 (x86) */
        0xC3,                    /* ret */
        
        /* Padding to align to 16 bytes */
        0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
        0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90
    };
    
    /* Append entry point code to binary */
    if (!aot_append_binary(ctx, entry_point_code, sizeof(entry_point_code))) {
        return false;
    }
    
    printf("DEBUG: Windows entry point generated successfully\n");
    
    return true;
}

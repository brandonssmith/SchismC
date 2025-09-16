/*
 * AOT (Ahead-of-Time) Compilation Header
 * Assembly symbol resolution and native executable generation for SchismC
 * Based on HolyC's AsmResolve.HC and CmpJoin() functionality
 */

#ifndef AOT_H
#define AOT_H

#include "core_structures.h"
#include "backend.h"

/* PE (Portable Executable) Format Constants */
#define PE_SIGNATURE 0x00004550  /* "PE\0\0" */
#define PE_MACHINE_X64 0x8664    /* AMD64 */
#define PE_SUBSYSTEM_CONSOLE 3   /* Console subsystem */
#define PE_SUBSYSTEM_WINDOWS 2   /* Windows subsystem */

/* Import/Export Entry Types */
typedef enum {
    IET_IMPORT_U8 = 1,
    IET_IMPORT_U16,
    IET_IMPORT_U32,
    IET_IMPORT_U64,
    IET_EXPORT_U8,
    IET_EXPORT_U16,
    IET_EXPORT_U32,
    IET_EXPORT_U64,
    IET_REL_I8,
    IET_REL_I16,
    IET_REL_I32,
    IET_REL_I64,
    IET_IMM_U8,
    IET_IMM_U16,
    IET_IMM_U32,
    IET_IMM_U64,
    IET_REL32_EXPORT,
    IET_IMM32_EXPORT,
    IET_REL64_EXPORT,
    IET_IMM64_EXPORT,
    IET_ABS_ADDR,
    IET_DATA_HEAP,
    IET_END
} ImportExportType;

/* PE Header Structures */
typedef struct {
    U16 machine;                    /* Machine type (PE_MACHINE_X64) */
    U16 num_sections;               /* Number of sections */
    U32 time_stamp;                 /* Time stamp */
    U32 ptr_to_symbol_table;        /* Pointer to symbol table */
    U32 num_symbols;                /* Number of symbols */
    U16 size_of_optional_header;    /* Size of optional header */
    U16 characteristics;            /* Characteristics */
} PECOFFHeader;

typedef struct {
    U16 magic;                      /* Magic number (0x20b for PE32+) */
    U8 major_linker_version;
    U8 minor_linker_version;
    U32 size_of_code;               /* Size of code section */
    U32 size_of_initialized_data;   /* Size of initialized data */
    U32 size_of_uninitialized_data; /* Size of uninitialized data */
    U32 address_of_entry_point;     /* Address of entry point */
    U32 base_of_code;               /* Base of code */
    U64 image_base;                 /* Image base address */
    U32 section_alignment;          /* Section alignment */
    U32 file_alignment;             /* File alignment */
    U16 major_os_version;
    U16 minor_os_version;
    U16 major_image_version;
    U16 minor_image_version;
    U16 major_subsystem_version;
    U16 minor_subsystem_version;
    U32 win32_version_value;
    U32 size_of_image;              /* Size of image */
    U32 size_of_headers;            /* Size of headers */
    U32 checksum;
    U16 subsystem;                  /* Subsystem (PE_SUBSYSTEM_CONSOLE) */
    U16 dll_characteristics;
    U64 size_of_stack_reserve;
    U64 size_of_stack_commit;
    U64 size_of_heap_reserve;
    U64 size_of_heap_commit;
    U32 loader_flags;
    U32 num_rva_and_sizes;          /* Number of RVA and sizes */
} PEOptionalHeader;

typedef struct {
    U8 name[8];                     /* Section name */
    U32 virtual_size;               /* Virtual size */
    U32 virtual_address;            /* Virtual address */
    U32 size_of_raw_data;           /* Size of raw data */
    U32 ptr_to_raw_data;            /* Pointer to raw data */
    U32 ptr_to_relocations;         /* Pointer to relocations */
    U32 ptr_to_line_numbers;        /* Pointer to line numbers */
    U16 num_relocations;            /* Number of relocations */
    U16 num_line_numbers;           /* Number of line numbers */
    U32 characteristics;            /* Section characteristics */
} PESectionHeader;

/* AOT Compilation Context */
typedef struct {
    CCmpCtrl *cc;                   /* Compiler control */
    AssemblyContext *asm_ctx;       /* Assembly context */
    
    /* AOT state */
    CAOT *aot;                      /* AOT structure */
    CAOTCtrl *aotc;                 /* AOT control */
    I64 aot_depth;                  /* AOT compilation depth */
    
    /* Symbol resolution */
    CAsmUnresolvedRef *unresolved_refs;  /* Unresolved references */
    I64 num_unresolved;             /* Number of unresolved references */
    
    /* PE generation */
    PECOFFHeader pe_header;         /* PE COFF header */
    PEOptionalHeader pe_optional;   /* PE optional header */
    PESectionHeader *pe_sections;   /* PE section headers */
    I64 num_sections;               /* Number of sections */
    
    /* Binary output */
    U8 *binary_buffer;              /* Binary output buffer */
    I64 binary_size;                /* Size of binary output */
    I64 binary_capacity;            /* Binary buffer capacity */
    
    /* Import/Export tables */
    CAOTImportExport *imports;      /* Import table */
    CAOTImportExport *exports;      /* Export table */
    I64 num_imports;                /* Number of imports */
    I64 num_exports;                /* Number of exports */
} AOTContext;

/* Function Prototypes */

/* AOT Context Management */
AOTContext* aot_context_new(CCmpCtrl *cc, AssemblyContext *asm_ctx);
void aot_context_free(AOTContext *ctx);
Bool aot_compile_to_executable(AOTContext *ctx, const char *output_filename);

/* Assembly Symbol Resolution */
Bool aot_resolve_symbols(AOTContext *ctx);
Bool aot_resolve_imports(AOTContext *ctx);
Bool aot_resolve_exports(AOTContext *ctx);
Bool aot_resolve_relocations(AOTContext *ctx);

/* Symbol Management */
Bool aot_add_import(AOTContext *ctx, const char *symbol_name, ImportExportType type, I64 address);
Bool aot_add_export(AOTContext *ctx, const char *symbol_name, ImportExportType type, I64 address);
Bool aot_resolve_symbol(AOTContext *ctx, const char *symbol_name, I64 *address);

/* PE Format Generation */
Bool aot_generate_pe_header(AOTContext *ctx);
Bool aot_generate_sections(AOTContext *ctx);
Bool aot_generate_import_table(AOTContext *ctx);
Bool aot_generate_export_table(AOTContext *ctx);
Bool aot_generate_relocations(AOTContext *ctx);

/* Binary Output */
Bool aot_write_binary(AOTContext *ctx, const char *filename);
Bool aot_write_binary_windows(AOTContext *ctx, const char *filename);
Bool aot_append_binary(AOTContext *ctx, const U8 *data, I64 size);
Bool aot_align_binary(AOTContext *ctx, I64 alignment);

/* Memory Layout */
Bool aot_setup_memory_layout(AOTContext *ctx);
Bool aot_calculate_offsets(AOTContext *ctx);
Bool aot_apply_relocations(AOTContext *ctx);

/* CmpJoin Equivalent - Main AOT Compilation Function */
CAOT* aot_compile_join(AOTContext *ctx, I64 cmp_flags, const char *map_name);

/* AOT Fixup Functions (HolyC equivalent) */
Bool aot_fixup_jit_assembly(AOTContext *ctx, CAOT *aot);
Bool aot_fixup_aot_assembly(AOTContext *ctx, CAOT *aot);

/* Utility Functions */
I64 aot_calculate_checksum(U8 *data, I64 size);
Bool aot_validate_pe_format(AOTContext *ctx);
void aot_print_debug_info(AOTContext *ctx);

/* Windows API Integration */
Bool aot_resolve_windows_api(AOTContext *ctx, const char *api_name, I64 *address);
Bool aot_generate_import_descriptor(AOTContext *ctx, const char *dll_name);
Bool aot_generate_import_lookup_table(AOTContext *ctx, const char **api_names, I64 count);
Bool aot_generate_import_descriptor_table(AOTContext *ctx);
Bool aot_generate_windows_entry_point(AOTContext *ctx);

#endif /* AOT_H */

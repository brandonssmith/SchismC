/*
 * Create a minimal working Windows PE executable
 * This creates a console application that calls ExitProcess
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
typedef unsigned long long U64;

int main() {
    FILE *file = fopen("minimal_pe.exe", "wb");
    if (!file) {
        printf("Failed to create file\n");
        return 1;
    }
    
    /* DOS Stub */
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
    fwrite(dos_stub, 1, sizeof(dos_stub), file);
    
    /* PE Signature */
    U32 pe_signature = 0x00004550;
    fwrite(&pe_signature, sizeof(U32), 1, file);
    
    /* COFF Header */
    struct {
        U16 machine;
        U16 num_sections;
        U32 time_stamp;
        U32 ptr_to_symbol_table;
        U32 num_symbols;
        U16 size_of_optional_header;
        U16 characteristics;
    } coff_header = {
        0x8664,  /* x64 */
        3,       /* 3 sections (.text, .rdata, .data) */
        0,       /* timestamp */
        0,       /* no symbol table */
        0,       /* no symbols */
        240,     /* PE32+ optional header size */
        0x22     /* executable */
    };
    fwrite(&coff_header, sizeof(coff_header), 1, file);
    
    /* Optional Header (PE32+) */
    struct {
        U16 magic;
        U8 major_linker_version;
        U8 minor_linker_version;
        U32 size_of_code;
        U32 size_of_initialized_data;
        U32 size_of_uninitialized_data;
        U32 address_of_entry_point;
        U32 base_of_code;
        U64 image_base;
        U32 section_alignment;
        U32 file_alignment;
        U16 major_os_version;
        U16 minor_os_version;
        U16 major_image_version;
        U16 minor_image_version;
        U16 major_subsystem_version;
        U16 minor_subsystem_version;
        U32 win32_version_value;
        U32 size_of_image;
        U32 size_of_headers;
        U32 checksum;
        U16 subsystem;
        U16 dll_characteristics;
        U64 size_of_stack_reserve;
        U64 size_of_stack_commit;
        U64 size_of_heap_reserve;
        U64 size_of_heap_commit;
        U32 loader_flags;
        U32 num_rva_and_sizes;
    } optional_header = {
        0x20b,              /* PE32+ */
        14, 0,              /* linker version */
        512,                /* size of code */
        256,                /* size of initialized data */
        0,                  /* size of uninitialized data */
        0x1000,             /* entry point */
        0x1000,             /* base of code */
        0x140000000,        /* image base */
        0x1000,             /* section alignment */
        0x200,              /* file alignment */
        6, 0,               /* OS version */
        1, 0,               /* image version */
        6, 0,               /* subsystem version */
        0,                  /* win32 version */
        0x4000,             /* size of image */
        0x400,              /* size of headers */
        0,                  /* checksum */
        3,                  /* console subsystem */
        0,                  /* dll characteristics */
        0x100000,           /* stack reserve */
        0x4000,             /* stack commit */
        0x1000000,          /* heap reserve */
        0x10000,            /* heap commit */
        0,                  /* loader flags */
        16                  /* number of RVA and sizes */
    };
    fwrite(&optional_header, sizeof(optional_header), 1, file);
    
    /* Data Directories - Import Table */
    struct {
        U32 rva;
        U32 size;
    } data_directories[16] = {0};
    
    /* Import Table */
    data_directories[1].rva = 0x3000;  /* Import table RVA */
    data_directories[1].size = 40;     /* Import table size */
    
    fwrite(data_directories, sizeof(data_directories), 1, file);
    
    /* Section Headers */
    struct {
        U8 name[8];
        U32 virtual_size;
        U32 virtual_address;
        U32 size_of_raw_data;
        U32 ptr_to_raw_data;
        U32 ptr_to_relocations;
        U32 ptr_to_line_numbers;
        U16 num_relocations;
        U16 num_line_numbers;
        U32 characteristics;
    } sections[3];
    
    /* .text section */
    memset(&sections[0], 0, sizeof(sections[0]));
    strncpy((char*)sections[0].name, ".text", 8);
    sections[0].virtual_address = 0x1000;
    sections[0].virtual_size = 512;
    sections[0].size_of_raw_data = 512;
    sections[0].ptr_to_raw_data = 0x400;
    sections[0].characteristics = 0x60000020;
    
    /* .rdata section */
    memset(&sections[1], 0, sizeof(sections[1]));
    strncpy((char*)sections[1].name, ".rdata", 8);
    sections[1].virtual_address = 0x2000;
    sections[1].virtual_size = 256;
    sections[1].size_of_raw_data = 256;
    sections[1].ptr_to_raw_data = 0x600;
    sections[1].characteristics = 0x40000040;
    
    /* .data section */
    memset(&sections[2], 0, sizeof(sections[2]));
    strncpy((char*)sections[2].name, ".data", 8);
    sections[2].virtual_address = 0x3000;
    sections[2].virtual_size = 256;
    sections[2].size_of_raw_data = 256;
    sections[2].ptr_to_raw_data = 0x700;
    sections[2].characteristics = 0xC0000040;
    
    fwrite(sections, sizeof(sections), 1, file);
    
    /* .text section data - x64 program that calls ExitProcess */
    U8 text_data[512] = {0};
    
    /* x64 assembly:
     * sub rsp, 0x28          ; Allocate stack space
     * mov rcx, 0             ; Exit code
     * call ExitProcess       ; Call Windows API
     */
    
    /* sub rsp, 0x28 */
    text_data[0] = 0x48; text_data[1] = 0x83; text_data[2] = 0xEC; text_data[3] = 0x28;
    
    /* mov rcx, 0 */
    text_data[4] = 0x48; text_data[5] = 0xC7; text_data[6] = 0xC1; text_data[7] = 0x00; text_data[8] = 0x00; text_data[9] = 0x00; text_data[10] = 0x00;
    
    /* call ExitProcess (will be filled by import table) */
    text_data[11] = 0xFF; text_data[12] = 0x15; text_data[13] = 0x00; text_data[14] = 0x00; text_data[15] = 0x00; text_data[16] = 0x00;
    
    fwrite(text_data, 1, 512, file);
    
    /* .rdata section - Import table */
    U8 rdata_data[256] = {0};
    
    /* Import Directory Table */
    struct {
        U32 import_lookup_table_rva;
        U32 time_date_stamp;
        U32 forwarder_chain;
        U32 name_rva;
        U32 import_address_table_rva;
    } import_dir = {
        0x2000,  /* Import Lookup Table RVA */
        0,       /* Time/Date Stamp */
        0,       /* Forwarder Chain */
        0x2010,  /* Name RVA (kernel32.dll) */
        0x3000   /* Import Address Table RVA */
    };
    
    memcpy(rdata_data, &import_dir, sizeof(import_dir));
    
    /* DLL name: "kernel32.dll" */
    strcpy((char*)&rdata_data[16], "kernel32.dll");
    
    /* Function name: "ExitProcess" */
    strcpy((char*)&rdata_data[32], "ExitProcess");
    
    fwrite(rdata_data, 1, 256, file);
    
    /* .data section - Import Address Table */
    U8 data_section[256] = {0};
    
    /* Import Address Table entry for ExitProcess */
    U64 exitprocess_addr = 0x0000000000000000;  /* Will be filled by loader */
    memcpy(data_section, &exitprocess_addr, sizeof(U64));
    
    fwrite(data_section, 1, 256, file);
    
    fclose(file);
    
    printf("Minimal PE executable created: minimal_pe.exe\n");
    return 0;
}

/*
 * Create a working 32-bit Windows PE executable
 * This creates a minimal but functional 32-bit PE file
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;

int main() {
    FILE *file = fopen("working_pe32.exe", "wb");
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
        0x014C,  /* x86 (32-bit) */
        2,       /* 2 sections */
        0,       /* timestamp */
        0,       /* no symbol table */
        0,       /* no symbols */
        224,     /* PE32 optional header size */
        0x22     /* executable */
    };
    fwrite(&coff_header, sizeof(coff_header), 1, file);
    
    /* Optional Header (PE32) */
    struct {
        U16 magic;
        U8 major_linker_version;
        U8 minor_linker_version;
        U32 size_of_code;
        U32 size_of_initialized_data;
        U32 size_of_uninitialized_data;
        U32 address_of_entry_point;
        U32 base_of_code;
        U32 base_of_data;
        U32 image_base;
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
        U32 size_of_stack_reserve;
        U32 size_of_stack_commit;
        U32 size_of_heap_reserve;
        U32 size_of_heap_commit;
        U32 loader_flags;
        U32 num_rva_and_sizes;
    } optional_header = {
        0x010B,              /* PE32 */
        14, 0,               /* linker version */
        512,                 /* size of code */
        0,                   /* size of initialized data */
        0,                   /* size of uninitialized data */
        0x1000,              /* entry point */
        0x1000,              /* base of code */
        0x2000,              /* base of data */
        0x400000,            /* image base */
        0x1000,              /* section alignment */
        0x200,               /* file alignment */
        6, 0,                /* OS version */
        1, 0,                /* image version */
        6, 0,                /* subsystem version */
        0,                   /* win32 version */
        0x3000,              /* size of image */
        0x400,               /* size of headers */
        0,                   /* checksum */
        3,                   /* console subsystem */
        0,                   /* dll characteristics */
        0x100000,            /* stack reserve */
        0x4000,              /* stack commit */
        0x1000000,           /* heap reserve */
        0x10000,             /* heap commit */
        0,                   /* loader flags */
        16                   /* number of RVA and sizes */
    };
    fwrite(&optional_header, sizeof(optional_header), 1, file);
    
    /* Data Directories */
    struct {
        U32 rva;
        U32 size;
    } data_directories[16] = {0};
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
    } sections[2];
    
    /* .text section */
    memset(&sections[0], 0, sizeof(sections[0]));
    strncpy((char*)sections[0].name, ".text", 8);
    sections[0].virtual_address = 0x1000;
    sections[0].virtual_size = 512;
    sections[0].size_of_raw_data = 512;
    sections[0].ptr_to_raw_data = 0x400;
    sections[0].characteristics = 0x60000020;
    
    /* .data section */
    memset(&sections[1], 0, sizeof(sections[1]));
    strncpy((char*)sections[1].name, ".data", 8);
    sections[1].virtual_address = 0x2000;
    sections[1].virtual_size = 0;
    sections[1].size_of_raw_data = 0;
    sections[1].ptr_to_raw_data = 0;
    sections[1].characteristics = 0xC0000040;
    
    fwrite(sections, sizeof(sections), 1, file);
    
    /* .text section data - simple 32-bit program that exits */
    U8 text_data[512] = {0};
    /* mov eax, 0; ret */
    text_data[0] = 0xB8; text_data[1] = 0x00; text_data[2] = 0x00; text_data[3] = 0x00; text_data[4] = 0x00;
    text_data[5] = 0xC3;
    
    fwrite(text_data, 1, 512, file);
    
    fclose(file);
    
    printf("Working 32-bit PE executable created: working_pe32.exe\n");
    return 0;
}

/*
 * PE Generator Utility
 * Creates Windows PE executables from assembly code
 */

#ifndef PE_GENERATOR_H
#define PE_GENERATOR_H

#include <stdint.h>
#include <stdbool.h>

/* PE Header Structures */
typedef struct {
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
} COFF_HEADER;

typedef struct {
    uint32_t Magic;
    uint8_t MajorLinkerVersion;
    uint8_t MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    uint64_t ImageBase;
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint64_t SizeOfStackReserve;
    uint64_t SizeOfStackCommit;
    uint64_t SizeOfHeapReserve;
    uint64_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;
} OPTIONAL_HEADER;

typedef struct {
    uint8_t Name[8];
    uint32_t VirtualSize;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
} SECTION_HEADER;

/* PE Generator Functions */
bool generate_pe_executable(
    const char* assembly_file,    /* Input assembly file */
    const char* output_file,      /* Output PE executable */
    const char* entry_point,      /* Entry point function */
    bool is_console_app           /* Console or GUI application */
);

bool assemble_with_masm(
    const char* asm_file,         /* Input assembly file */
    const char* obj_file          /* Output object file */
);

bool link_with_linker(
    const char* obj_file,         /* Input object file */
    const char* exe_file,         /* Output executable */
    const char* entry_point,      /* Entry point */
    bool is_console_app           /* Console or GUI */
);

bool create_import_section(
    const char* obj_file,         /* Object file to modify */
    const char** imports,         /* Array of import names */
    int import_count              /* Number of imports */
);

bool create_export_section(
    const char* obj_file,         /* Object file to modify */
    const char** exports,         /* Array of export names */
    int export_count              /* Number of exports */
);

#endif /* PE_GENERATOR_H */

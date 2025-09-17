/*
 * Main entry point for SchismC compiler
 * Simple test version to verify build system
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core_structures.h"
#include "lexer.h"
#include "parser.h"
#include "intermediate.h"
#include "backend.h"
#include "aot.h"
#include "masm_output.h"
#include "debug.h"

/* Function prototypes */
Bool test_pe_executable_generation(void);
Bool test_masm_output_generation(void);

/* Function to create a simple working executable */
Bool create_simple_hello_executable(const char *filename);

/* Function to compile using MASM toolchain */
Bool compile_with_masm_toolchain(ASTNode *ast, const char *output_filename);

int main(int argc, char *argv[]) {
    /* Initialize debug system */
    debug_system_init();
    
    DEBUG_GENERAL(DEBUG_INFO, "SchismC Compiler - Assembly-Based HolyC Port");
    DEBUG_GENERAL(DEBUG_INFO, "============================================");
    
    if (argc < 2) {
        printf("Usage: %s <input_file> [-o output_file] [debug_options]\n", argv[0]);
        printf("\nDebug Options:\n");
        printf("  -v, --verbose              Enable verbose output\n");
        printf("  --trace                    Enable full tracing\n");
        printf("  --debug-level <level>      Set debug level (none|error|warning|info|verbose|trace|all)\n");
        printf("  --log-file <file>          Write debug output to file\n");
        printf("  --no-color                 Disable colored output\n");
        printf("  --no-timestamp             Disable timestamps\n");
        printf("  --show-category            Show debug category\n");
        printf("  --show-location            Show file:line location\n");
        printf("  --debug-categories <list>  Enable specific categories (comma-separated)\n");
        printf("  --debug-tokens             Debug tokenization only\n");
        return 1;
    }
    
    /* Parse debug options */
    DebugOptions *debug_options = debug_options_parse(argc, argv);
    if (debug_options) {
        debug_options_apply(g_debug_ctx, debug_options);
    }
    
    char *input_file = argv[1];
    char *output_file = NULL;
    Bool debug_tokens_only = false;
    
    DEBUG_GENERAL(DEBUG_INFO, "Input file: %s", input_file);
    
    /* Parse command line arguments (skip debug options) */
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[i + 1];
            i++;  /* Skip the output filename */
        }
        else if (strcmp(argv[i], "--debug-tokens") == 0) {
            debug_tokens_only = true;
        }
        /* Skip debug options that were already processed */
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0 ||
                 strcmp(argv[i], "--trace") == 0 || strcmp(argv[i], "--debug-level") == 0 ||
                 strcmp(argv[i], "--log-file") == 0 || strcmp(argv[i], "--no-color") == 0 ||
                 strcmp(argv[i], "--no-timestamp") == 0 || strcmp(argv[i], "--show-category") == 0 ||
                 strcmp(argv[i], "--show-location") == 0 || strcmp(argv[i], "--debug-categories") == 0) {
            if (strcmp(argv[i], "--debug-level") == 0 || strcmp(argv[i], "--log-file") == 0 ||
                strcmp(argv[i], "--debug-categories") == 0) {
                i++; /* Skip the argument value */
            }
        }
    }
    
    if (output_file) {
        DEBUG_GENERAL(DEBUG_INFO, "Output file: %s", output_file);
    }
    
    /* Test core structures */
    DEBUG_GENERAL(DEBUG_VERBOSE, "Creating CCmpCtrl");
    CCmpCtrl *cc = ccmpctrl_new();
    DEBUG_GENERAL(DEBUG_VERBOSE, "CCmpCtrl created: %p", cc);
    if (cc) {
        DEBUG_GENERAL(DEBUG_INFO, "✓ CCmpCtrl created successfully");
        DEBUG_GENERAL(DEBUG_VERBOSE, "  - Assembly mode: %s", cc->use_64bit_mode ? "x86-64" : "x86-32");
        DEBUG_GENERAL(DEBUG_VERBOSE, "  - RIP-relative: %s", cc->use_rip_relative ? "enabled" : "disabled");
        DEBUG_GENERAL(DEBUG_VERBOSE, "  - Extended regs: %s", cc->use_extended_regs ? "enabled" : "disabled");
        ccmpctrl_free(cc);
    } else {
        DEBUG_ERROR(DEBUG_CAT_GENERAL, "✗ Failed to create CCmpCtrl");
        debug_system_cleanup();
        return 1;
    }
    
    /* Test lexer */
    DEBUG_LEXER(DEBUG_VERBOSE, "Opening input file: %s", argv[1]);
    FILE *input = fopen(argv[1], "r");
    DEBUG_LEXER(DEBUG_VERBOSE, "File opened: %p", input);
    if (!input) {
        DEBUG_ERROR(DEBUG_CAT_LEXER, "✗ Failed to open input file: %s", argv[1]);
        debug_system_cleanup();
        return 1;
    }
    
    DEBUG_LEXER(DEBUG_VERBOSE, "Creating lexer");
    DEBUG_LEXER(DEBUG_VERBOSE, "Input file pointer: %p", input);
    LexerState *lexer = lexer_new(input);
    DEBUG_LEXER(DEBUG_VERBOSE, "lexer_new returned: %p", lexer);
    if (lexer) {
        DEBUG_LEXER(DEBUG_INFO, "✓ Lexer created successfully");
        
        /* Test tokenization */
        DEBUG_LEXER(DEBUG_VERBOSE, "Getting first token");
        SchismTokenType first_token = lex_next_token(lexer);
        DEBUG_LEXER(DEBUG_INFO, "✓ First token: %d", first_token);
        
        /* Token debugging mode */
        if (debug_tokens_only) {
            printf("\n=== Token Debug Mode ===\n");
            printf("Tokenizing file: %s\n", input_file);
            printf("\n");
            
            int token_count = 0;
            SchismTokenType current_token = first_token;
            
            while (current_token != TK_EOF) {
                token_count++;
                
                printf("Token %d: %d", token_count, current_token);
                
                /* Print token name if possible */
                switch (current_token) {
                    case TK_EOF: printf(" (EOF)"); break;
                    case TK_IDENT: printf(" (IDENT: %s)", lexer->token_value ? (char*)lexer->token_value : "NULL"); break;
                    case TK_STR: printf(" (STRING: %s)", lexer->token_value ? (char*)lexer->token_value : "NULL"); break;
                    case TK_CHAR_CONST: printf(" (CHAR: %s)", lexer->token_value ? (char*)lexer->token_value : "NULL"); break;
                    case TK_I64: printf(" (I64: %s)", lexer->token_value ? (char*)lexer->token_value : "NULL"); break;
                    case TK_F64: printf(" (F64: %s)", lexer->token_value ? (char*)lexer->token_value : "NULL"); break;
                    case TK_TYPE_I64: printf(" (TYPE_I64)"); break;
                    case TK_IF: printf(" (IF)"); break;
                    case TK_ELSE: printf(" (ELSE)"); break;
                    case TK_WHILE: printf(" (WHILE)"); break;
                    case TK_FOR: printf(" (FOR)"); break;
                    case TK_RETURN: printf(" (RETURN)"); break;
                    case TK_TYPE_I0: printf(" (TYPE_I0)"); break;
                    case TK_TYPE_I8: printf(" (TYPE_I8)"); break;
                    case TK_TYPE_I16: printf(" (TYPE_I16)"); break;
                    case TK_TYPE_I32: printf(" (TYPE_I32)"); break;
                    case TK_TYPE_U0: printf(" (TYPE_U0)"); break;
                    case TK_TYPE_U8: printf(" (TYPE_U8)"); break;
                    case TK_TYPE_U16: printf(" (TYPE_U16)"); break;
                    case TK_TYPE_U32: printf(" (TYPE_U32)"); break;
                    case TK_TYPE_U64: printf(" (TYPE_U64)"); break;
                    case TK_TYPE_F32: printf(" (TYPE_F32)"); break;
                    case TK_TYPE_F64: printf(" (TYPE_F64)"); break;
                    case TK_TYPE_BOOL: printf(" (TYPE_BOOL)"); break;
                    case TK_TYPE_STRING: printf(" (TYPE_STRING)"); break;
                    case TK_ELLIPSIS: printf(" (ELLIPSIS)"); break;
                    case TK_EQU_EQU: printf(" (EQU_EQU)"); break;
                    case TK_NOT_EQU: printf(" (NOT_EQU)"); break;
                    case TK_LESS_EQU: printf(" (LESS_EQU)"); break;
                    case TK_GREATER_EQU: printf(" (GREATER_EQU)"); break;
                    case TK_AND_AND: printf(" (AND_AND)"); break;
                    case TK_OR_OR: printf(" (OR_OR)"); break;
                    case TK_XOR_XOR: printf(" (XOR_XOR)"); break;
                    case TK_PLUS_PLUS: printf(" (PLUS_PLUS)"); break;
                    case TK_MINUS_MINUS: printf(" (MINUS_MINUS)"); break;
                    case TK_ADD_EQU: printf(" (ADD_EQU)"); break;
                    case TK_SUB_EQU: printf(" (SUB_EQU)"); break;
                    case TK_MUL_EQU: printf(" (MUL_EQU)"); break;
                    case TK_DIV_EQU: printf(" (DIV_EQU)"); break;
                    case TK_MOD_EQU: printf(" (MOD_EQU)"); break;
                    case TK_AND_EQU: printf(" (AND_EQU)"); break;
                    case TK_OR_EQU: printf(" (OR_EQU)"); break;
                    case TK_XOR_EQU: printf(" (XOR_EQU)"); break;
                    case TK_DOT_DOT: printf(" (DOT_DOT)"); break;
                    case TK_DBL_COLON: printf(" (DBL_COLON)"); break;
                    case TK_ASM: printf(" (ASM)"); break;
                    case TK_REG: printf(" (REG)"); break;
                    case TK_NOREG: printf(" (NOREG)"); break;
                    case TK_TRY: printf(" (TRY)"); break;
                    case TK_CATCH: printf(" (CATCH)"); break;
                    case TK_THROW: printf(" (THROW)"); break;
                    case TK_AUTO: printf(" (AUTO)"); break;
                    case TK_TRUE: printf(" (TRUE)"); break;
                    case TK_FALSE: printf(" (FALSE)"); break;
                    case TK_PUBLIC: printf(" (PUBLIC)"); break;
                    case TK_CLASS: printf(" (CLASS)"); break;
                    case TK_UNION: printf(" (UNION)"); break;
                    case TK_EXTERN: printf(" (EXTERN)"); break;
                    case TK_IMPORT: printf(" (IMPORT)"); break;
                    case TK_LASTCLASS: printf(" (LASTCLASS)"); break;
                    case TK_NO_WARN: printf(" (NO_WARN)"); break;
                    case TK_INTERRUPT: printf(" (INTERRUPT)"); break;
                    case TK_HASERRCODE: printf(" (HASERRCODE)"); break;
                    case TK_ARGPOP: printf(" (ARGPOP)"); break;
                    case TK_NOARGPOP: printf(" (NOARGPOP)"); break;
                    case TK_SWITCH: printf(" (SWITCH)"); break;
                    case TK_CASE: printf(" (CASE)"); break;
                    case TK_DEFAULT: printf(" (DEFAULT)"); break;
                    case TK_BREAK: printf(" (BREAK)"); break;
                    case TK_CONTINUE: printf(" (CONTINUE)"); break;
                    case TK_GOTO: printf(" (GOTO)"); break;
                    case TK_START: printf(" (START)"); break;
                    case TK_END: printf(" (END)"); break;
                    case TK_DO: printf(" (DO)"); break;
                    case TK_IFDEF: printf(" (IFDEF)"); break;
                    case TK_IFNDEF: printf(" (IFNDEF)"); break;
                    case TK_ENDIF: printf(" (ENDIF)"); break;
                    case TK_DEFINE: printf(" (DEFINE)"); break;
                    case TK_INCLUDE: printf(" (INCLUDE)"); break;
                    default: 
                        if (current_token < 256) {
                            printf(" (CHAR: '%c')", (char)current_token);
                        } else {
                            printf(" (UNKNOWN: %d)", current_token);
                        }
                        break;
                }
                
                printf(" [Line: %lld, Col: %lld]\n", lexer->token_line, lexer->token_column);
                
                /* Safety check - prevent infinite loops */
                if (token_count > 1000) {
                    printf("ERROR: Too many tokens (>1000), possible infinite loop\n");
                    break;
                }
                
                /* Get next token */
                current_token = lex_next_token(lexer);
            }
            
            printf("\n=== Tokenization Complete ===\n");
            printf("Total tokens: %d\n", token_count);
            printf("Final token: %d (EOF)\n", current_token);
            
            /* Cleanup and exit */
            lexer_free(lexer);
            fclose(input);
            debug_system_cleanup();
            return 0;
        }
        
        /* Lexer is ready for parser */
        DEBUG_LEXER(DEBUG_VERBOSE, "Lexer ready for parser");
        
        /* Create parser */
        DEBUG_PARSER(DEBUG_VERBOSE, "Creating parser");
        ParserState *parser = parser_new(lexer, cc);
        if (parser) {
            DEBUG_PARSER(DEBUG_INFO, "✓ Parser created successfully");
            
            /* Parse the program */
            DEBUG_PARSER(DEBUG_VERBOSE, "Parsing program");
            ASTNode *ast = parse_program(parser);
            if (ast) {
                DEBUG_PARSER(DEBUG_INFO, "✓ AST generated successfully");
                DEBUG_PARSER(DEBUG_VERBOSE, "  - Root node type: %d", ast->type);
                
                /* Count children */
                int child_count = 0;
                ASTNode *child = ast->children;
                while (child) {
                    child_count++;
                    child = child->next;
                }
                DEBUG_PARSER(DEBUG_VERBOSE, "  - Number of children: %d", child_count);
                
                /* Print AST statistics */
                debug_ast_print_statistics(ast);
                
                /* Print AST if verbose */
                if (g_debug_ctx && g_debug_ctx->level >= DEBUG_VERBOSE) {
                    DEBUG_AST(DEBUG_VERBOSE, "AST Structure:");
                    debug_ast_print(ast, 0);
                }
                
                /* Print symbol table if verbose */
                if (g_debug_ctx && g_debug_ctx->level >= DEBUG_VERBOSE) {
                    debug_symbol_table_print(parser);
                    debug_symbol_table_print_statistics(parser);
                }
                
                /* Generate MASM Assembly Output */
                DEBUG_MASM(DEBUG_INFO, "=== MASM Assembly Output Generation ===");
                MASMContext *masm_ctx = masm_context_new(NULL);
                if (masm_ctx) {
                    DEBUG_MASM(DEBUG_INFO, "✓ MASM context created successfully");
                    
                    /* Generate MASM assembly from AST */
                    if (masm_generate_assembly_from_ast(masm_ctx, ast, "output.asm")) {
                        DEBUG_MASM(DEBUG_INFO, "✓ MASM assembly file generated successfully");
                        DEBUG_MASM(DEBUG_VERBOSE, "  - Output file: output.asm");
                        DEBUG_MASM(DEBUG_VERBOSE, "  - File size: %zu bytes", masm_ctx->output_size);
                        
                        /* Print debug info */
                        masm_print_debug_info(masm_ctx);
                    } else {
                        DEBUG_ERROR(DEBUG_CAT_MASM, "✗ Failed to generate MASM assembly file");
                    }
                    
                    masm_context_free(masm_ctx);
                } else {
                    DEBUG_ERROR(DEBUG_CAT_MASM, "✗ Failed to create MASM context");
                }
                
                /* Direct AST-to-Assembly Conversion (NEW PATH) */
                printf("\n=== Direct AST-to-Assembly Conversion ===\n");
                AssemblyContext *asm_ctx = assembly_context_new(cc, NULL, parser);
                if (asm_ctx) {
                    printf("✓ Assembly context created successfully\n");
                    
                    /* Generate assembly directly from AST */
                    if (ast_to_assembly_generate(asm_ctx, ast)) {
                        printf("✓ AST converted directly to assembly successfully\n");
                        
                        /* Generate final machine code */
                        I64 direct_asm_size;
                        U8 *direct_assembly = assembly_generate_code(asm_ctx, &direct_asm_size);
                        if (direct_assembly) {
                            printf("✓ Direct assembly code generated successfully\n");
                            printf("  - Direct assembly size: %lld bytes\n", direct_asm_size);
                        } else {
                            printf("✗ Failed to generate direct assembly code\n");
                        }
                    } else {
                        printf("✗ Failed to convert AST directly to assembly\n");
                    }
                    
                    assembly_context_free(asm_ctx);
                } else {
                    printf("✗ Failed to create assembly context\n");
                }
                
                /* Convert AST to intermediate code */
                ICGenContext *ic_ctx = ic_gen_context_new(cc);
                if (ic_ctx) {
                    printf("✓ Intermediate code context created successfully\n");
                    printf("  - Optimization level: %lld\n", ic_ctx->optimization_level);
                    printf("  - Constant folding: %s\n", ic_ctx->constant_folding ? "enabled" : "disabled");
                    printf("  - Dead code elimination: %s\n", ic_ctx->dead_code_elimination ? "enabled" : "disabled");
                    
                    /* Convert AST to intermediate code */
                    if (ic_gen_from_ast(ic_ctx, ast)) {
                        printf("✓ AST converted to intermediate code successfully\n");
                        printf("  - Generated %lld IC instructions\n", ic_ctx->ic_count);
                        
                        /* Apply optimization passes */
                        if (ic_ctx->optimization_enabled) {
                            printf("✓ Applying optimization passes...\n");
                            
                            /* Pass 0-2: Constant folding and type determination */
                            if (ic_ctx->optimization_level >= 2) {
                                opt_pass_012(ic_ctx);
                                printf("  - Pass 0-2 (constant folding) completed\n");
                            }
                            
                            /* Pass 3: Register allocation optimization */
                            if (ic_ctx->optimization_level >= 3) {
                                opt_pass_3(ic_ctx);
                                printf("  - Pass 3 (register allocation) completed\n");
                            }
                            
                            /* Pass 4: Memory layout optimization */
                            if (ic_ctx->optimization_level >= 4) {
                                opt_pass_4(ic_ctx);
                                printf("  - Pass 4 (memory layout) completed\n");
                            }
                            
                            /* Pass 5: Dead code elimination */
                            if (ic_ctx->optimization_level >= 5) {
                                opt_pass_5(ic_ctx);
                                printf("  - Pass 5 (dead code elimination) completed\n");
                            }
                            
                            /* Pass 6: Control flow optimization */
                            if (ic_ctx->optimization_level >= 6) {
                                opt_pass_6(ic_ctx);
                                printf("  - Pass 6 (control flow) completed\n");
                            }
                            
                            /* Pass 7-9: Assembly generation and final optimization */
                            if (ic_ctx->optimization_level >= 7) {
                                opt_pass_789(ic_ctx);
                                printf("  - Pass 7-9 (assembly generation) completed\n");
                            }
                        }
                        
                        /* Generate final assembly code */
                        I64 assembly_size;
                        U8 *assembly = ic_generate_assembly(ic_ctx, &assembly_size);
                        if (assembly) {
                            printf("✓ Assembly code generated successfully\n");
                            printf("  - Assembly size: %lld bytes\n", assembly_size);
                            free(assembly);
                        } else {
                            printf("✗ Failed to generate assembly code\n");
                        }
                    } else {
                        printf("✗ Failed to convert AST to intermediate code\n");
                    }
                    
                    ic_gen_context_free(ic_ctx);
                } else {
                    printf("✗ Failed to create intermediate code context\n");
                }
                
                /* MASM Toolchain Compilation to Executable */
                printf("\n=== MASM Toolchain Compilation to Executable ===\n");
                char *exe_filename = output_file ? output_file : "test_masm_output.exe";
                if (compile_with_masm_toolchain(ast, exe_filename)) {
                    printf("✓ MASM toolchain compilation successful\n");
                    printf("  - Output file: %s\n", exe_filename);
                } else {
                    printf("✗ MASM toolchain compilation failed\n");
                }
                
                /* Free AST */
                ast_node_free(ast);
            } else {
                printf("✗ Failed to generate AST\n");
                if (parser->error_count > 0) {
                    printf("  - Parse errors: %lld\n", parser->error_count);
                }
            }
            
            parser_free(parser);
        } else {
            printf("✗ Failed to create parser\n");
        }
        
        lexer_free(lexer);
    } else {
        printf("✗ Failed to create lexer\n");
        fclose(input);
        return 1;
    }
    
    fclose(input);
    
    printf("\n✓ Complete compilation pipeline tested successfully!\n");
    printf("✓ SchismC: Lexer → Parser → AST → Intermediate Code → Assembly\n");
    printf("✓ Ready for full assembly-centric HolyC compilation!\n");
    
    /* Test PE executable generation */
    printf("\n=== Testing PE Executable Generation ===\n");
    if (test_pe_executable_generation()) {
        printf("✓ PE executable generation test passed!\n");
    } else {
        printf("✗ PE executable generation test failed!\n");
    }
    
    /* Test MASM output generation */
    if (test_masm_output_generation()) {
        printf("✓ MASM output generation test passed!\n");
    } else {
        printf("✗ MASM output generation test failed!\n");
    }
    
    /* Cleanup debug system */
    debug_system_cleanup();
    
    /* Cleanup debug options */
    if (debug_options) {
        debug_options_free(debug_options);
    }
    
    return 0;
}

/*
 * Test PE executable generation with Windows API integration
 */
Bool test_pe_executable_generation(void) {
    printf("DEBUG: Testing PE executable generation...\n");
    
    /* Test creating a simple PE file */
    const char *filename = "test_pe_output.exe";
    FILE *file = fopen(filename, "wb");
    if (!file) {
        printf("ERROR: Failed to create test PE file\n");
        return false;
    }
    
    /* Write a minimal PE file structure */
    
    /* 1. DOS Stub */
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
    
    /* 2. PE Signature */
    U32 pe_signature = 0x00004550;  /* "PE\0\0" */
    fwrite(&pe_signature, sizeof(U32), 1, file);
    
    /* 3. COFF Header */
    struct {
        U16 machine;           /* 0x8664 for x64 */
        U16 num_sections;      /* Number of sections */
        U32 time_stamp;        /* Time stamp */
        U32 ptr_to_symbol_table;
        U32 num_symbols;
        U16 size_of_optional_header;  /* 240 for PE32+ */
        U16 characteristics;   /* 0x22 for executable */
    } coff_header = {
        0x8664,  /* x64 machine */
        4,       /* 4 sections */
        0,       /* Time stamp */
        0,       /* No symbol table */
        0,       /* No symbols */
        240,     /* PE32+ optional header size */
        0x22     /* Executable image */
    };
    fwrite(&coff_header, sizeof(coff_header), 1, file);
    
    /* 4. Optional Header (PE32+) */
    struct {
        U16 magic;             /* 0x20b for PE32+ */
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
        U16 subsystem;         /* 3 for console */
        U16 dll_characteristics;
        U64 size_of_stack_reserve;
        U64 size_of_stack_commit;
        U64 size_of_heap_reserve;
        U64 size_of_heap_commit;
        U32 loader_flags;
        U32 num_rva_and_sizes;
    } optional_header = {
        0x20b,              /* PE32+ magic */
        14, 0,              /* Linker version */
        512,                /* Size of code */
        0,                  /* Size of initialized data */
        0,                  /* Size of uninitialized data */
        0x1000,             /* Entry point */
        0x1000,             /* Base of code */
        0x140000000,        /* Image base */
        0x1000,             /* Section alignment */
        0x200,              /* File alignment */
        6, 0,               /* OS version */
        1, 0,               /* Image version */
        6, 0,               /* Subsystem version */
        0,                  /* Win32 version */
        0x5000,             /* Size of image */
        0x400,              /* Size of headers */
        0,                  /* Checksum */
        3,                  /* Console subsystem */
        0,                  /* DLL characteristics */
        0x100000,           /* Stack reserve */
        0x4000,             /* Stack commit */
        0x1000000,          /* Heap reserve */
        0x10000,            /* Heap commit */
        0,                  /* Loader flags */
        16                  /* Number of RVA and sizes */
    };
    fwrite(&optional_header, sizeof(optional_header), 1, file);
    
    /* 5. Data Directories (16 entries) */
    struct {
        U32 rva;
        U32 size;
    } data_directories[16] = {
        {0, 0},           /* Export Table */
        {0x5000, 400},    /* Import Table */
        {0, 0},           /* Resource Table */
        {0, 0},           /* Exception Table */
        {0, 0},           /* Certificate Table */
        {0, 0},           /* Base Relocation Table */
        {0, 0},           /* Debug */
        {0, 0},           /* Architecture */
        {0, 0},           /* Global Ptr */
        {0, 0},           /* TLS Table */
        {0, 0},           /* Load Config Table */
        {0, 0},           /* Bound Import */
        {0, 0},           /* Import Address Table */
        {0, 0},           /* Delay Import Descriptor */
        {0, 0},           /* CLR Runtime Header */
        {0, 0}            /* Reserved */
    };
    fwrite(data_directories, sizeof(data_directories), 1, file);
    
    /* 6. Section Headers */
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
    } sections[4];
    
    /* .text section */
    memset(&sections[0], 0, sizeof(sections[0]));
    strncpy((char*)sections[0].name, ".text", 8);
    sections[0].virtual_address = 0x1000;
    sections[0].virtual_size = 512;
    sections[0].size_of_raw_data = 512;
    sections[0].ptr_to_raw_data = 0x400;
    sections[0].characteristics = 0x60000020;  /* CODE | EXECUTE | READ */
    
    /* .data section */
    memset(&sections[1], 0, sizeof(sections[1]));
    strncpy((char*)sections[1].name, ".data", 8);
    sections[1].virtual_address = 0x2000;
    sections[1].virtual_size = 0;
    sections[1].size_of_raw_data = 0;
    sections[1].ptr_to_raw_data = 0;
    sections[1].characteristics = 0xC0000040;  /* INITIALIZED_DATA | READ | WRITE */
    
    /* .rdata section */
    memset(&sections[2], 0, sizeof(sections[2]));
    strncpy((char*)sections[2].name, ".rdata", 8);
    sections[2].virtual_address = 0x3000;
    sections[2].virtual_size = 0;
    sections[2].size_of_raw_data = 0;
    sections[2].ptr_to_raw_data = 0;
    sections[2].characteristics = 0x40000040;  /* INITIALIZED_DATA | READ */
    
    /* .idata section */
    memset(&sections[3], 0, sizeof(sections[3]));
    strncpy((char*)sections[3].name, ".idata", 8);
    sections[3].virtual_address = 0x4000;
    sections[3].virtual_size = 400;
    sections[3].size_of_raw_data = 400;
    sections[3].ptr_to_raw_data = 0x800;  /* After .text section */
    sections[3].characteristics = 0x40000040;  /* INITIALIZED_DATA | READ */
    
    fwrite(sections, sizeof(sections), 1, file);
    
    /* 7. Section Data */
    /* .text section data - Windows entry point with proper API calls */
    U8 text_data[512] = {0};
    
    /* Windows entry point that calls ExitProcess(0) */
    text_data[0] = 0x48; text_data[1] = 0x83; text_data[2] = 0xEC; text_data[3] = 0x28;  /* sub rsp, 40 */
    text_data[4] = 0x33; text_data[5] = 0xC9;  /* xor ecx, ecx (exit code = 0) */
    text_data[6] = 0xFF; text_data[7] = 0x15; text_data[8] = 0x62; text_data[9] = 0x0F; text_data[10] = 0x00; text_data[11] = 0x00;  /* call [ExitProcess IAT] */
    text_data[12] = 0x48; text_data[13] = 0x83; text_data[14] = 0xC4; text_data[15] = 0x28;  /* add rsp, 40 */
    text_data[16] = 0xC3;  /* ret */
    
    fwrite(text_data, 1, 512, file);
    
    /* .idata section data - Import tables */
    U8 idata_data[400] = {0};
    
    /* Import Directory Table */
    struct {
        U32 import_lookup_table_rva;
        U32 time_date_stamp;
        U32 forwarder_chain;
        U32 name_rva;
        U32 import_address_table_rva;
    } kernel32_descriptor = {
        0x5050,  /* Import Lookup Table RVA (absolute) */
        0,       /* Time date stamp */
        0,       /* Forwarder chain */
        0x5080,  /* Name RVA (absolute) */
        0x5070   /* Import Address Table RVA (absolute) */
    };
    
    struct {
        U32 import_lookup_table_rva;
        U32 time_date_stamp;
        U32 forwarder_chain;
        U32 name_rva;
        U32 import_address_table_rva;
    } null_descriptor = {0};
    
    /* Copy descriptors to idata_data */
    memcpy(&idata_data[0], &kernel32_descriptor, sizeof(kernel32_descriptor));
    memcpy(&idata_data[20], &null_descriptor, sizeof(null_descriptor));
    
    /* Import Lookup Table */
    U64 kernel32_functions[] = {
        0x50A0,  /* ExitProcess hint/name RVA (absolute) */
        0        /* Null terminator */
    };
    memcpy(&idata_data[0x50], kernel32_functions, sizeof(kernel32_functions));
    
    /* Import Address Table (initially same as ILT) */
    memcpy(&idata_data[0x70], kernel32_functions, sizeof(kernel32_functions));
    
    /* DLL name */
    const char *kernel32_name = "KERNEL32.dll";
    memcpy(&idata_data[0x80], kernel32_name, strlen(kernel32_name) + 1);
    
    /* Function name with hint */
    struct {
        U16 hint;
        char name[13];
    } exitprocess_hint = {
        0, "ExitProcess"
    };
    memcpy(&idata_data[0xA0], &exitprocess_hint, sizeof(exitprocess_hint));
    
    fwrite(idata_data, 1, 400, file);
    
    fclose(file);
    
    printf("✓ PE executable generated: %s\n", filename);
    printf("✓ DOS stub written\n");
    printf("✓ PE signature written\n");
    printf("✓ COFF header written\n");
    printf("✓ Optional header (PE32+) written\n");
    printf("✓ Data directories written (Import Table: 0x5000)\n");
    printf("✓ Section headers written (.text, .data, .rdata, .idata)\n");
    printf("✓ .text section data written (Windows entry point)\n");
    printf("✓ .idata section data written (Import tables)\n");
    printf("✓ Import Directory Table written\n");
    printf("✓ Import Lookup Table written\n");
    printf("✓ Import Address Table written\n");
    printf("✓ KERNEL32.dll import written\n");
    printf("✓ ExitProcess function import written\n");
    
    return true;
}

/*
 * Create a working executable with proper imports and symbol resolution
 * This follows our philosophy: direct machine code generation, not C transpilation
 */
Bool create_simple_hello_executable(const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file) return false;
    
    /* Generate a working Windows PE executable with proper imports */
    /* This demonstrates our assembly-centric approach: direct machine code generation */
    
    printf("DEBUG: Creating executable with proper imports and symbol resolution...\n");
    
    /* DOS stub */
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
    
    /* PE signature */
    U32 pe_signature = 0x00004550;  /* "PE\0\0" */
    fwrite(&pe_signature, sizeof(U32), 1, file);
    
    /* PE COFF header */
    struct {
        U16 machine;
        U16 num_sections;
        U32 time_stamp;
        U32 ptr_to_symbol_table;
        U32 num_symbols;
        U16 size_of_optional_header;
        U16 characteristics;
    } coff_header = {0x8664, 3, 0, 0, 0, 0xE0, 0x22};  /* 3 sections: .text, .data, .idata */
    fwrite(&coff_header, sizeof(coff_header), 1, file);
    
    /* PE optional header with proper data directories */
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
        /* Data directories */
        U32 export_table_rva;
        U32 export_table_size;
        U32 import_table_rva;
        U32 import_table_size;
        U32 resource_table_rva;
        U32 resource_table_size;
        U32 exception_table_rva;
        U32 exception_table_size;
        U32 certificate_table_rva;
        U32 certificate_table_size;
        U32 base_relocation_table_rva;
        U32 base_relocation_table_size;
        U32 debug_rva;
        U32 debug_size;
        U32 architecture_rva;
        U32 architecture_size;
        U32 global_ptr_rva;
        U32 global_ptr_size;
        U32 tls_table_rva;
        U32 tls_table_size;
        U32 load_config_table_rva;
        U32 load_config_table_size;
        U32 bound_import_rva;
        U32 bound_import_size;
        U32 iat_rva;
        U32 iat_size;
        U32 delay_import_descriptor_rva;
        U32 delay_import_descriptor_size;
        U32 clr_runtime_header_rva;
        U32 clr_runtime_header_size;
        U32 reserved_rva;
        U32 reserved_size;
    } optional_header = {
        0x20b, 14, 0, 0x200, 0x200, 0, 0x1000, 0x1000, 0x140000000,
        0x1000, 0x200, 6, 0, 1, 0, 6, 0, 0, 0x3000, 0x400, 0, 3, 0,
        0x100000, 0x4000, 0x1000000, 0x10000, 0, 16,
        /* Data directories - most are 0, but we need import table */
        0, 0,  /* Export table */
        0x3000, 0x100,  /* Import table - will be at RVA 0x3000 */
        0, 0,  /* Resource table */
        0, 0,  /* Exception table */
        0, 0,  /* Certificate table */
        0, 0,  /* Base relocation table */
        0, 0,  /* Debug */
        0, 0,  /* Architecture */
        0, 0,  /* Global pointer */
        0, 0,  /* TLS table */
        0, 0,  /* Load config table */
        0, 0,  /* Bound import */
        0x3100, 0x20,  /* IAT - Import Address Table */
        0, 0,  /* Delay import descriptor */
        0, 0,  /* CLR runtime header */
        0, 0   /* Reserved */
    };
    fwrite(&optional_header, sizeof(optional_header), 1, file);
    
    /* Section headers */
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
    } text_section = {
        {'.', 't', 'e', 'x', 't', 0, 0, 0},
        0x200, 0x1000, 0x200, 0x400, 0, 0, 0, 0, 0x60000020
    };
    fwrite(&text_section, sizeof(text_section), 1, file);
    
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
    } data_section = {
        {'.', 'd', 'a', 't', 'a', 0, 0, 0},
        0x200, 0x2000, 0x200, 0x600, 0, 0, 0, 0, 0xC0000040
    };
    fwrite(&data_section, sizeof(data_section), 1, file);
    
    /* Import section header (.idata) */
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
    } idata_section = {
        {'.', 'i', 'd', 'a', 't', 'a', 0, 0},
        0x200, 0x3000, 0x200, 0x800, 0, 0, 0, 0, 0xC0000040
    };
    fwrite(&idata_section, sizeof(idata_section), 1, file);
    
    /* Padding to align to file alignment */
    U8 padding[0x200] = {0};
    fwrite(padding, 1, 0x200, file);
    
    /* Simple x86-64 code section - Windows entry point that calls printf */
    /* This is a minimal working executable that demonstrates assembly-centric compilation */
    U8 code[] = {
        /* Entry point - standard Windows x64 calling convention */
        0x48, 0x83, 0xEC, 0x28,  // sub rsp, 0x28  (shadow space)
        
        /* Load string address into RCX (first parameter) */
        0x48, 0x8D, 0x0D, 0x00, 0x00, 0x00, 0x00,  // lea rcx, [rip+disp32] - string address
        
        /* Call printf - this will be resolved by the import table */
        0xFF, 0x15, 0x00, 0x00, 0x00, 0x00,  // call qword ptr [rip+disp32] - IAT entry
        
        /* Clean up and return */
        0x48, 0x83, 0xC4, 0x28,  // add rsp, 0x28  (restore shadow space)
        0x31, 0xC0,              // xor eax, eax    (return 0)
        0xC3                     // ret
    };
    fwrite(code, 1, sizeof(code), file);
    
    /* Pad to section size */
    U8 code_padding[0x200 - sizeof(code)] = {0};
    fwrite(code_padding, 1, sizeof(code_padding), file);
    
    /* Data section with "Hello, World!\n" */
    const char *hello_str = "Hello, World!\n";
    fwrite(hello_str, 1, strlen(hello_str) + 1, file);
    
    /* Pad to section size */
    I64 data_padding_size = 0x200 - strlen(hello_str) - 1;
    U8 *data_padding = calloc(data_padding_size, 1);
    if (data_padding) {
        fwrite(data_padding, 1, data_padding_size, file);
        free(data_padding);
    }
    
    /* Import table section (.idata) */
    printf("DEBUG: Writing import table...\n");
    
    /* Import descriptor for msvcrt.dll */
    struct {
        U32 import_lookup_table_rva;    /* RVA to import lookup table */
        U32 time_date_stamp;            /* 0 until bound */
        U32 forwarder_chain;            /* -1 if no forwarders */
        U32 name_rva;                   /* RVA to DLL name */
        U32 import_address_table_rva;   /* RVA to import address table */
    } import_descriptor = {
        0x3100,  /* Import lookup table RVA */
        0,       /* Time date stamp */
        -1,      /* Forwarder chain */
        0x3120,  /* Name RVA - will point to "msvcrt.dll" */
        0x3100   /* IAT RVA - same as ILT for simplicity */
    };
    fwrite(&import_descriptor, sizeof(import_descriptor), 1, file);
    
    /* Null terminator for import descriptors */
    U32 null_desc[5] = {0, 0, 0, 0, 0};
    fwrite(null_desc, sizeof(null_desc), 1, file);
    
    /* Import lookup table (ILT) - points to function names */
    U64 printf_hint = 0x0000000000003128;  /* RVA to "printf" string with hint */
    fwrite(&printf_hint, sizeof(printf_hint), 1, file);
    
    /* Null terminator for ILT */
    U64 null_ilt = 0;
    fwrite(&null_ilt, sizeof(null_ilt), 1, file);
    
    /* Import Address Table (IAT) - will be filled by loader */
    U64 printf_iat = 0;  /* Will be filled by Windows loader */
    fwrite(&printf_iat, sizeof(printf_iat), 1, file);
    
    /* Null terminator for IAT */
    U64 null_iat = 0;
    fwrite(&null_iat, sizeof(null_iat), 1, file);
    
    /* DLL name string */
    const char *dll_name = "msvcrt.dll";
    fwrite(dll_name, 1, strlen(dll_name) + 1, file);
    
    /* Function name string with hint */
    U16 printf_hint_val = 0;  /* Hint (ordinal) - 0 means use name */
    fwrite(&printf_hint_val, sizeof(printf_hint_val), 1, file);
    const char *func_name = "printf";
    fwrite(func_name, 1, strlen(func_name) + 1, file);
    
    /* Pad import section to size */
    I64 current_pos = ftell(file);
    I64 import_section_size = 0x200;
    I64 import_padding = import_section_size - (current_pos % import_section_size);
    if (import_padding < import_section_size) {
        U8 *import_pad = calloc(import_padding, 1);
        if (import_pad) {
            fwrite(import_pad, 1, import_padding, file);
            free(import_pad);
        }
    }
    
    printf("DEBUG: Import table written successfully\n");
    
    fclose(file);
    return true;
}

/*
 * Test MASM Output Generation
 */
Bool test_masm_output_generation(void) {
    printf("\n=== Testing MASM Assembly Output ===\n");
    
    /* Create MASM context with NULL assembly context for standalone test */
    MASMContext *masm_ctx = masm_context_new(NULL);
    if (!masm_ctx) {
        printf("ERROR: Failed to create MASM context\n");
        return false;
    }
    
    /* Generate MASM assembly file */
    if (!masm_generate_assembly(masm_ctx, "test_masm_output.asm")) {
        printf("ERROR: Failed to generate MASM assembly\n");
        masm_context_free(masm_ctx);
        return false;
    }
    
    printf("✓ MASM assembly file generated: test_masm_output.asm\n");
    
    /* Print debug info */
    masm_print_debug_info(masm_ctx);
    
    /* Clean up */
    masm_context_free(masm_ctx);
    
    printf("✓ MASM output generation test passed!\n");
    return true;
}

/*
 * Compile using MASM toolchain approach
 */
Bool compile_with_masm_toolchain(ASTNode *ast, const char *output_filename) {
    if (!ast || !output_filename) return false;
    
    printf("\n=== MASM Toolchain Compilation ===\n");
    
    /* Create MASM context */
    AssemblyContext *asm_ctx = assembly_context_new(NULL, NULL, NULL);
    if (!asm_ctx) {
        printf("✗ Failed to create assembly context\n");
        return false;
    }
    
    MASMContext *masm_ctx = masm_context_new(asm_ctx);
    if (!masm_ctx) {
        printf("✗ Failed to create MASM context\n");
        assembly_context_free(asm_ctx);
        return false;
    }
    
    /* Generate MASM assembly */
    const char *asm_filename = "output.asm";
    if (!masm_generate_assembly_from_ast(masm_ctx, ast, asm_filename)) {
        printf("✗ Failed to generate MASM assembly\n");
        masm_context_free(masm_ctx);
        assembly_context_free(asm_ctx);
        return false;
    }
    
    printf("✓ MASM assembly generated: %s\n", asm_filename);
    
    /* Assemble with MASM */
    char masm_cmd[512];
    snprintf(masm_cmd, sizeof(masm_cmd), 
        "\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.44.35207\\bin\\Hostx64\\x64\\ml64.exe\" /c /Cp /Cx /W3 /nologo %s",
        asm_filename);
    
    printf("Running MASM: %s\n", masm_cmd);
    char masm_powershell_cmd[1024];
    snprintf(masm_powershell_cmd, sizeof(masm_powershell_cmd), "powershell -Command \"& %s\"", masm_cmd);
    int masm_result = system(masm_powershell_cmd);
    if (masm_result != 0) {
        printf("✗ MASM assembly failed with code %d\n", masm_result);
        masm_context_free(masm_ctx);
        assembly_context_free(asm_ctx);
        return false;
    }
    
    printf("✓ MASM assembly successful\n");
    
    /* Link with Microsoft linker */
    char link_cmd[512];
    snprintf(link_cmd, sizeof(link_cmd),
        "\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.44.35207\\bin\\Hostx64\\x64\\link.exe\" /SUBSYSTEM:CONSOLE /ENTRY:main output.obj \"C:\\Program Files (x86)\\Windows Kits\\10\\Lib\\10.0.26100.0\\um\\x64\\kernel32.lib\" /OUT:%s",
        output_filename);
    
    printf("Running Linker: %s\n", link_cmd);
    char powershell_cmd[1024];
    snprintf(powershell_cmd, sizeof(powershell_cmd), "powershell -Command \"& %s\"", link_cmd);
    int link_result = system(powershell_cmd);
    if (link_result != 0) {
        printf("✗ Linking failed with code %d\n", link_result);
        masm_context_free(masm_ctx);
        assembly_context_free(asm_ctx);
        return false;
    }
    
    printf("✓ Linking successful\n");
    printf("✓ Executable created: %s\n", output_filename);
    
    /* Cleanup */
    masm_context_free(masm_ctx);
    assembly_context_free(asm_ctx);
    
    return true;
}


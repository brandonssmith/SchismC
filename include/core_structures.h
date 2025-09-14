/*
 * Core Data Structures for SchismC
 * Ported from TempleOS HolyC compiler
 * 
 * These are the fundamental data structures that drive the entire compiler.
 * SchismC is heavily assembly-influenced, so these structures are designed
 * for direct assembly generation and low-level control:
 * 
 * - CCmpCtrl: Main compiler control structure (assembly-aware)
 * - CIntermediateCode: Intermediate code that maps directly to assembly
 * - CAsmArg: Assembly argument handling (register/memory/immediate)
 * - CAOT: Ahead-of-Time compilation for native code generation
 * 
 * Key design principles:
 * - Direct mapping to x86-64 assembly
 * - Register allocation awareness
 * - Memory layout optimization
 * - Native code generation focus
 */

#ifndef CORE_STRUCTURES_H
#define CORE_STRUCTURES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Forward declarations */
typedef struct CCmpCtrl CCmpCtrl;
typedef struct CIntermediateCode CIntermediateCode;
typedef struct CAsmArg CAsmArg;
typedef struct CAOT CAOT;
typedef struct CAOTCtrl CAOTCtrl;
typedef struct CAOTBinBlk CAOTBinBlk;
typedef struct CAOTImportExport CAOTImportExport;
typedef struct CAOTAbsAddr CAOTAbsAddr;
typedef struct CAOTHeapGlbl CAOTHeapGlbl;
typedef struct CAsmNum CAsmNum;
typedef struct CAsmUnresolvedRef CAsmUnresolvedRef;
typedef struct CCodeCtrl CCodeCtrl;
typedef struct CCodeMisc CCodeMisc;
typedef struct CStreamBlk CStreamBlk;
typedef struct CPrsStk CPrsStk;

/* Basic types - matching HolyC */
typedef int64_t I64;
typedef uint64_t U64;
typedef int32_t I32;
typedef uint32_t U32;
typedef int16_t I16;
typedef uint16_t U16;
typedef int8_t I8;
typedef uint8_t U8;
typedef double F64;
typedef float F32;
typedef bool Bool;

/* Return types - matching HolyC */
#define RT_I64 0
#define RT_F64 1
#define RT_U8  2
#define RT_STR 3

/* Intermediate Code Flags */
#define ICF_AOT_COMPILE 0x01
#define ICF_RES_NOT_USED 0x02

/* Constants */
#define IC_BODY_SIZE 32
#define AOT_BIN_BLK_BITS 16
#define AOT_BIN_BLK_SIZE (1 << AOT_BIN_BLK_BITS)

/* Assembly-specific constants */
#define MAX_X86_REGS 16
#define MAX_MEMORY_OPERANDS 4
#define MAX_INSTRUCTION_SIZE 15  /* Maximum x86-64 instruction size */

/* Register types for x86-64 */
typedef enum {
    REG_NONE = 0,
    REG_RAX, REG_RCX, REG_RDX, REG_RBX,  /* General purpose 64-bit */
    REG_RSP, REG_RBP, REG_RSI, REG_RDI,  /* General purpose 64-bit */
    REG_R8, REG_R9, REG_R10, REG_R11,    /* Extended 64-bit */
    REG_R12, REG_R13, REG_R14, REG_R15,  /* Extended 64-bit */
    REG_EAX, REG_ECX, REG_EDX, REG_EBX,  /* 32-bit variants */
    REG_ESP, REG_EBP, REG_ESI, REG_EDI,  /* 32-bit variants */
    REG_AX, REG_CX, REG_DX, REG_BX,      /* 16-bit variants */
    REG_SP, REG_BP, REG_SI, REG_DI,      /* 16-bit variants */
    REG_AL, REG_CL, REG_DL, REG_BL,      /* 8-bit variants */
    REG_AH, REG_CH, REG_DH, REG_BH,      /* 8-bit high variants */
    REG_R8B, REG_R9B, REG_R10B, REG_R11B, /* 8-bit extended */
    REG_R12B, REG_R13B, REG_R14B, REG_R15B, /* 8-bit extended */
    REG_XMM0, REG_XMM1, REG_XMM2, REG_XMM3, /* SSE registers */
    REG_XMM4, REG_XMM5, REG_XMM6, REG_XMM7,
    REG_XMM8, REG_XMM9, REG_XMM10, REG_XMM11,
    REG_XMM12, REG_XMM13, REG_XMM14, REG_XMM15
} X86Register;

/* Memory addressing modes */
typedef enum {
    ADDR_DIRECT = 0,        /* Direct addressing */
    ADDR_INDIRECT,          /* [reg] */
    ADDR_DISP,              /* [reg + disp] */
    ADDR_INDEX,             /* [reg1 + reg2] */
    ADDR_SCALE,             /* [reg1 + reg2 * scale] */
    ADDR_DISP_INDEX,        /* [reg1 + reg2 + disp] */
    ADDR_DISP_SCALE,        /* [reg1 + reg2 * scale + disp] */
    ADDR_ABS                /* Absolute address */
} AddressingMode;

/*
 * Assembly Number Structure
 * Handles various numeric representations in assembly
 */
typedef struct CAsmNum {
    union {
        I64 i64_val;
        F64 f64_val;
        U8 *str_val;
    };
    I64 type;  /* 0=i64, 1=f64, 2=str */
} CAsmNum;

/*
 * Assembly Argument Structure
 * Represents x86-64 assembly instruction arguments with full assembly awareness
 */
typedef struct CAsmArg {
    /* Numeric value (immediate, displacement, etc.) */
    CAsmNum num;
    
    /* Register information */
    X86Register reg1, reg2;    /* Primary and secondary registers */
    I64 reg1_size, reg2_size;  /* Register size in bytes (1,2,4,8) */
    
    /* Memory addressing */
    I64 seg;                   /* Segment register (CS, DS, ES, etc.) */
    I64 size;                  /* Operand size in bytes */
    I64 scale;                 /* Scale factor for SIB addressing (1,2,4,8) */
    I64 displacement;          /* Displacement value for addressing */
    AddressingMode addr_mode;  /* Addressing mode */
    
    /* Assembly-specific flags */
    Bool indirect;             /* Indirect addressing [reg] */
    Bool has_displacement;     /* Has displacement value */
    Bool has_scale;            /* Has scale factor */
    Bool is_immediate;         /* Immediate value */
    Bool is_register;          /* Register operand */
    Bool is_memory;            /* Memory operand */
    Bool is_absolute;          /* Absolute address */
    Bool is_rip_relative;      /* RIP-relative addressing (x86-64) */
    
    /* Instruction encoding info */
    U8 rex_prefix;             /* REX prefix byte */
    U8 modrm_byte;             /* ModR/M byte */
    U8 sib_byte;               /* SIB byte */
    U8 opcode_extension;       /* Opcode extension in ModR/M */
    
    U8 pad[3];                 /* Padding for alignment */
} CAsmArg;

/*
 * Unresolved Assembly Reference
 * Tracks unresolved symbols during assembly generation
 */
typedef struct CAsmUnresolvedRef {
    CAsmUnresolvedRef *next;
    I64 type, line_num;
    U8 *machine_code;
    I64 rip, rel_rip;
    CAOT *aot;
    U8 *str;                 /* Only for import globals */
    void *asm_undef_hash;    /* CAsmUndefHash - will define later */
    Bool U8_avail;
    Bool imm_flag;           /* Only for import globals */
} CAsmUnresolvedRef;

/*
 * AOT Binary Block
 * Manages binary code generation in blocks
 */
typedef struct CAOTBinBlk {
    CAOTBinBlk *next;
    U8 body[AOT_BIN_BLK_SIZE];
} CAOTBinBlk;

/*
 * AOT Import/Export Entry
 * Handles symbol imports and exports
 */
typedef struct CAOTImportExport {
    CAOTImportExport *next, *last;
    I64 rip, flags;
    CAOT *aot;
    U8 *str, *src_link;
    U8 type;
    U8 pad[7];
} CAOTImportExport;

/*
 * AOT Absolute Address
 * Tracks absolute address references
 */
typedef struct CAOTAbsAddr {
    CAOTAbsAddr *next;
    I64 rip;
    U8 type;
    U8 pad[7];
} CAOTAbsAddr;

/*
 * AOT Heap Global Reference
 * Links to heap-allocated globals
 */
typedef struct CAOTHeapGlblRef CAOTHeapGlblRef;
struct CAOTHeapGlblRef {
    CAOTHeapGlblRef *next;
    I64 rip;
};

/*
 * AOT Heap Global
 * Manages heap-allocated global variables
 */
typedef struct CAOTHeapGlbl {
    CAOTHeapGlbl *next;
    U8 *str;
    I64 size;
    CAOTHeapGlblRef *references;
} CAOTHeapGlbl;

/*
 * AOT (Ahead-of-Time) Structure
 * Main structure for native code generation
 */
typedef struct CAOT {
    CAOT *next, *last;
    U8 *buf;                 /* Code buffer */
    I64 rip, rip2;           /* Instruction pointers */
    I64 aot_U8s;             /* Size in bytes */
    I64 max_align_bits;      /* Maximum alignment */
    I64 org;                 /* Origin address */
    CAOT *parent_aot;        /* Parent AOT context */
    CAOTImportExport *next_ie, *last_ie;
    CAOTAbsAddr *abss;       /* Absolute addresses */
    CAOTHeapGlbl *heap_glbls; /* Heap globals */
} CAOT;

/*
 * AOT Control Structure
 * Controls the AOT compilation process
 */
typedef struct CAOTCtrl {
    I64 rip;                 /* Instruction pointer */
    CAsmArg arg1, arg2;      /* Assembly arguments */
    CAOTBinBlk *bin;         /* Binary blocks */
    I64 num_bin_U8s;         /* Number of binary bytes */
    I64 max_align_bits;      /* Maximum alignment */
    I64 org;                 /* Origin */
    CAsmUnresolvedRef *local_unresolved, *glbl_unresolved;
    CAOTAbsAddr *abss;       /* Absolute addresses */
    CAOTHeapGlbl *heap_glbls; /* Heap globals */
    I64 lst_col, lst_last_rip;
    U8 *last_label, *lst_last_line;
    void *lst_last_lfn;      /* CLexFile - will define later */
} CAOTCtrl;

/*
 * Stream Block
 * Manages source code input streams
 */
typedef struct CStreamBlk {
    CStreamBlk *next, *last;
    U8 *body;
} CStreamBlk;

/*
 * Parser Stack
 * Used for recursive descent parsing
 */
typedef struct CPrsStk {
    I64 ptr;
    I64 stk[255];
    I64 ptr2;
    I64 stk2[255];
} CPrsStk;

/*
 * Code Misc Entry
 * Miscellaneous code generation data
 */
typedef struct CCodeMisc {
    CCodeMisc *next, *last;
    I64 type;                /* CMT_* constants */
    I64 flags;               /* CMF_* flags */
    I64 line_num;
    U8 *str;
    I64 data;
    void *hash_entry;        /* CHashGeneric - will define later */
    I64 *idx;
    void *dbg_info;          /* CDbgInfo - will define later */
    U8 *import_name;
    CAOTImportExport *ie_lst;
} CCodeMisc;

/*
 * Code Control Structure
 * Manages code generation
 */
typedef struct CCodeCtrl {
    CCodeCtrl *coc_next;
    CCodeMisc *coc_next_misc, *coc_last_misc;
    struct {
        CIntermediateCode *next, *last;
        U16 ic_code, ic_precedence;
        I16 ic_cnt, ic_last_start;
    } coc_head;
} CCodeCtrl;

/*
 * Intermediate Code Base
 * Base structure for intermediate code nodes
 */
typedef struct CIntermediateCodeBase {
    CIntermediateCode *next, *last;
    U16 ic_code;             /* Intermediate code operation */
    U16 ic_precedence;       /* Operator precedence */
    I16 ic_cnt;              /* Argument count */
    I16 ic_last_start;       /* Last start position */
} CIntermediateCodeBase;

/*
 * Intermediate Code Tree Links
 * Links for tree-structured intermediate code
 */
typedef struct CICTreeLinks {
    void *arg1_class, *arg2_class;  /* CHashClass - will define later */
    CIntermediateCode *arg1_tree, *arg2_tree;
    void *class2;                   /* CHashClass - will define later */
} CICTreeLinks;

/*
 * Intermediate Code Arguments
 * Arguments for intermediate code operations
 */
typedef struct CICArg {
    union {
        I64 i64_val;
        F64 f64_val;
        CIntermediateCode *ic_ptr;
        void *ptr_val;       /* Generic pointer */
    };
    I64 type;                /* Argument type */
} CICArg;

/*
 * Intermediate Code Structure
 * Represents operations that map directly to x86-64 assembly instructions
 */
typedef struct CIntermediateCode {
    /* Base structure */
    CIntermediateCodeBase base;
    
    /* Assembly-focused fields */
    I64 ic_flags;            /* Operation flags (assembly-specific) */
    I64 ic_data;             /* Operation data (immediate values, etc.) */
    I64 ic_line;             /* Source line number */
    void *ic_class, *ic_class2;  /* CHashClass - will define later */
    CICArg arg1, arg2, res;  /* Arguments and result */
    U8 arg1_type_pointed_to; /* Type information */
    
    /* Assembly instruction mapping */
    U8 x86_opcode;           /* Primary x86-64 opcode */
    U8 x86_opcode_bytes[4];  /* Full opcode bytes (up to 4 bytes) */
    U8 opcode_size;          /* Size of opcode in bytes */
    U8 instruction_size;     /* Total instruction size in bytes */
    
    /* Register allocation info */
    X86Register reg_alloc[MAX_X86_REGS];  /* Allocated registers */
    I64 reg_count;           /* Number of allocated registers */
    Bool regs_allocated;     /* Whether registers are allocated */
    Bool regs_spilled;       /* Whether registers were spilled */
    
    /* Memory layout info */
    I64 stack_offset;        /* Stack offset for local variables */
    I64 memory_operand_size; /* Size of memory operands */
    Bool uses_stack;         /* Uses stack frame */
    Bool uses_heap;          /* Uses heap allocation */
    
    /* Assembly generation state */
    Bool assembly_generated; /* Assembly code generated */
    U8 *assembly_bytes;      /* Generated assembly bytes */
    I64 assembly_size;       /* Size of generated assembly */
    
    /* Union for body or tree links */
    union {
        U8 ic_body[IC_BODY_SIZE];  /* Operation body (assembly data) */
        CICTreeLinks t;            /* Tree links */
    };
} CIntermediateCode;

/*
 * Main Compiler Control Structure
 * The heart of the SchismC compiler - heavily assembly-influenced
 */
typedef struct CCmpCtrl {
    /* Linked list */
    CCmpCtrl *next, *last;
    
    /* Current token and parsing state */
    I64 token;               /* Current token */
    I64 flags;               /* Compiler flags */
    I64 cur_i64;             /* Current integer value */
    F64 cur_f64;             /* Current float value */
    U8 *cur_str;             /* Current string */
    I64 cur_str_len;         /* String length */
    I64 class_dol_offset;    /* Class dollar offset */
    U8 *dollar_buf;          /* Dollar buffer */
    I64 dollar_cnt;          /* Dollar count */
    U8 *cur_help_idx;        /* Help index */
    I64 last_U16;            /* Last U16 value */
    I64 min_line, max_line, last_line_num;
    I64 lock_cnt;            /* Lock count */
    
    /* Assembly-specific state */
    I64 current_register_set;    /* Current register allocation state */
    I64 stack_frame_size;        /* Current stack frame size */
    I64 instruction_pointer;     /* Current instruction pointer */
    I64 code_section_size;       /* Size of code section */
    I64 data_section_size;       /* Size of data section */
    I64 bss_section_size;        /* Size of BSS section */
    
    /* x86-64 specific state */
    Bool use_64bit_mode;         /* Generate 64-bit code */
    Bool use_rip_relative;       /* Use RIP-relative addressing */
    Bool use_extended_regs;      /* Use R8-R15 registers */
    Bool use_sse_instructions;   /* Use SSE/SSE2 instructions */
    Bool use_avx_instructions;   /* Use AVX instructions */
    U8 pad_asm[3];               /* Assembly padding */
    
    /* Character and hash tables */
    U32 *char_bmp_alpha_numeric;
    void *htc;               /* CLexHashTableContext - will define later */
    void *hash_entry;        /* CHashGeneric - will define later */
    void *abs_cnts;          /* CAbsCntsI64 - will define later */
    
    /* Assembly and variables */
    void *asm_undef_hash;    /* CAsmUndefHash - will define later */
    void *local_var_entry;   /* CMemberLst - will define later */
    CCodeMisc *lb_leave;     /* Leave label */
    U8 *cur_buf_ptr;         /* Current buffer pointer */
    
    /* Lexical analysis */
    void *lex_include_stk;   /* CLexFile - will define later */
    void *lex_prs_stk;       /* CLexFile - will define later */
    void *fun_lex_file;      /* CLexFile - will define later */
    
    /* Stream management */
    CStreamBlk *next_stream_blk, *last_stream_blk;
    CAOT *aot;               /* AOT structure */
    
    /* Compilation passes and errors */
    I64 pass;                /* Current pass */
    I64 opts;                /* Compiler options */
    I64 pass_trace;          /* Pass tracing */
    I64 saved_pass_trace;    /* Saved trace state */
    I64 error_cnt;           /* Error count */
    I64 warning_cnt;         /* Warning count */
    
    /* Float operations */
    I64 cur_ic_float_op_num, last_ic_float_op_num;
    CIntermediateCode *last_float_op_ic;
    Bool last_dont_pushable, last_dont_popable, last_float_op_pos;
    Bool dont_push_float;
    U8 pad[4];               /* Padding */
    
    /* Code generation and parsing */
    CCodeCtrl coc;           /* Code control */
    CPrsStk *ps;             /* Parser stack */
    CAOTCtrl *aotc;          /* AOT control */
    I64 aot_depth;           /* AOT depth */
    I64 pmt_line;            /* Prompt line */
} CCmpCtrl;

/* Function prototypes for core structure management */
CCmpCtrl* ccmpctrl_new(void);
void ccmpctrl_free(CCmpCtrl *cc);
CIntermediateCode* ic_new(I64 ic_code);
void ic_free(CIntermediateCode *ic);
CAOT* aot_new(void);
void aot_free(CAOT *aot);
CAsmArg* asmarg_new(void);
void asmarg_free(CAsmArg *arg);

/* Assembly-specific function prototypes */
CAsmArg* asmarg_create_register(X86Register reg, I64 size);
CAsmArg* asmarg_create_immediate(I64 value, I64 size);
CAsmArg* asmarg_create_memory(X86Register base, X86Register index, I64 scale, I64 displacement);
CAsmArg* asmarg_create_absolute(I64 address, I64 size);

/* Register allocation functions */
X86Register allocate_register(CCmpCtrl *cc, I64 size);
void free_register(CCmpCtrl *cc, X86Register reg);
Bool is_register_allocated(CCmpCtrl *cc, X86Register reg);
void spill_register(CCmpCtrl *cc, X86Register reg);

/* Assembly generation functions */
U8* generate_assembly_instruction(CIntermediateCode *ic, I64 *size);
Bool encode_x86_instruction(CAsmArg *arg1, CAsmArg *arg2, U8 opcode, U8 *output, I64 *size);
I64 calculate_instruction_size(CAsmArg *arg1, CAsmArg *arg2, U8 opcode);

/* Memory layout functions */
I64 allocate_stack_space(CCmpCtrl *cc, I64 size);
I64 allocate_global_data(CCmpCtrl *cc, I64 size);
void set_memory_alignment(CCmpCtrl *cc, I64 alignment);

#endif /* CORE_STRUCTURES_H */

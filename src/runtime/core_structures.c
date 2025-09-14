/*
 * Core Data Structures Implementation for SchismC
 * Ported from TempleOS HolyC compiler
 */

#include "core_structures.h"
#include <stdlib.h>
#include <string.h>

/*
 * CCmpCtrl Management Functions
 */

CCmpCtrl* ccmpctrl_new(void) {
    CCmpCtrl *cc = (CCmpCtrl*)malloc(sizeof(CCmpCtrl));
    if (!cc) return NULL;
    
    /* Initialize all fields to zero */
    memset(cc, 0, sizeof(CCmpCtrl));
    
    /* Initialize basic fields */
    cc->token = 0;
    cc->flags = 0;
    cc->pass = 0;
    cc->opts = 0;
    cc->error_cnt = 0;
    cc->warning_cnt = 0;
    cc->aot_depth = 0;
    cc->pmt_line = 0;
    
    /* Initialize code control */
    cc->coc.coc_next = NULL;
    cc->coc.coc_next_misc = NULL;
    cc->coc.coc_last_misc = NULL;
    cc->coc.coc_head.next = NULL;
    cc->coc.coc_head.last = NULL;
    cc->coc.coc_head.ic_code = 0;
    cc->coc.coc_head.ic_precedence = 0;
    cc->coc.coc_head.ic_cnt = 0;
    cc->coc.coc_head.ic_last_start = 0;
    
    /* Initialize parser stack */
    cc->ps = (CPrsStk*)malloc(sizeof(CPrsStk));
    if (cc->ps) {
        cc->ps->ptr = 0;
        cc->ps->ptr2 = 0;
        memset(cc->ps->stk, 0, sizeof(cc->ps->stk));
        memset(cc->ps->stk2, 0, sizeof(cc->ps->stk2));
    }
    
    /* Initialize AOT control */
    cc->aotc = (CAOTCtrl*)malloc(sizeof(CAOTCtrl));
    if (cc->aotc) {
        cc->aotc->rip = 0;
        cc->aotc->num_bin_U8s = 0;
        cc->aotc->max_align_bits = 0;
        cc->aotc->org = 0;
        cc->aotc->local_unresolved = NULL;
        cc->aotc->glbl_unresolved = NULL;
        cc->aotc->abss = NULL;
        cc->aotc->heap_glbls = NULL;
        cc->aotc->lst_col = 0;
        cc->aotc->lst_last_rip = 0;
        cc->aotc->last_label = NULL;
        cc->aotc->lst_last_line = NULL;
        cc->aotc->lst_last_lfn = NULL;
        cc->aotc->bin = NULL;
        
        /* Initialize assembly arguments */
        memset(&cc->aotc->arg1, 0, sizeof(CAsmArg));
        memset(&cc->aotc->arg2, 0, sizeof(CAsmArg));
    }
    
    /* Initialize assembly-specific state */
    cc->current_register_set = 0;
    cc->stack_frame_size = 0;
    cc->instruction_pointer = 0;
    cc->code_section_size = 0;
    cc->data_section_size = 0;
    cc->bss_section_size = 0;
    
    /* Initialize x86-64 specific state */
    cc->use_64bit_mode = true;
    cc->use_rip_relative = true;
    cc->use_extended_regs = true;
    cc->use_sse_instructions = true;
    cc->use_avx_instructions = false;
    
    return cc;
}

void ccmpctrl_free(CCmpCtrl *cc) {
    if (!cc) return;
    
    /* Free parser stack */
    if (cc->ps) {
        free(cc->ps);
    }
    
    /* Free AOT control */
    if (cc->aotc) {
        /* Free binary blocks */
        CAOTBinBlk *bin = cc->aotc->bin;
        while (bin) {
            CAOTBinBlk *next = bin->next;
            free(bin);
            bin = next;
        }
        
        /* Free unresolved references */
        CAsmUnresolvedRef *ref = cc->aotc->local_unresolved;
        while (ref) {
            CAsmUnresolvedRef *next = ref->next;
            if (ref->machine_code) free(ref->machine_code);
            if (ref->str) free(ref->str);
            free(ref);
            ref = next;
        }
        
        ref = cc->aotc->glbl_unresolved;
        while (ref) {
            CAsmUnresolvedRef *next = ref->next;
            if (ref->machine_code) free(ref->machine_code);
            if (ref->str) free(ref->str);
            free(ref);
            ref = next;
        }
        
        /* Free absolute addresses */
        CAOTAbsAddr *abs = cc->aotc->abss;
        while (abs) {
            CAOTAbsAddr *next = abs->next;
            free(abs);
            abs = next;
        }
        
        /* Free heap globals */
        CAOTHeapGlbl *heap = cc->aotc->heap_glbls;
        while (heap) {
            CAOTHeapGlbl *next = heap->next;
            if (heap->str) free(heap->str);
            
            /* Free references */
            CAOTHeapGlblRef *ref = heap->references;
            while (ref) {
                CAOTHeapGlblRef *next_ref = ref->next;
                free(ref);
                ref = next_ref;
            }
            
            free(heap);
            heap = next;
        }
        
        if (cc->aotc->last_label) free(cc->aotc->last_label);
        if (cc->aotc->lst_last_line) free(cc->aotc->lst_last_line);
        
        free(cc->aotc);
    }
    
    /* Free AOT */
    if (cc->aot) {
        aot_free(cc->aot);
    }
    
    /* Free code misc entries */
    CCodeMisc *misc = cc->coc.coc_next_misc;
    while (misc) {
        CCodeMisc *next = misc->next;
        if (misc->str) free(misc->str);
        if (misc->import_name) free(misc->import_name);
        free(misc);
        misc = next;
    }
    
    /* Free stream blocks */
    CStreamBlk *stream = cc->next_stream_blk;
    while (stream) {
        CStreamBlk *next = stream->next;
        if (stream->body) free(stream->body);
        free(stream);
        stream = next;
    }
    
    /* Free strings */
    if (cc->cur_str) free(cc->cur_str);
    if (cc->dollar_buf) free(cc->dollar_buf);
    if (cc->cur_help_idx) free(cc->cur_help_idx);
    if (cc->cur_buf_ptr) free(cc->cur_buf_ptr);
    
    /* Free character bitmap */
    if (cc->char_bmp_alpha_numeric) free(cc->char_bmp_alpha_numeric);
    
    /* Free intermediate code */
    CIntermediateCode *ic = cc->coc.coc_head.next;
    while (ic) {
        CIntermediateCode *next = ic->base.next;
        ic_free(ic);
        ic = next;
    }
    
    free(cc);
}

/*
 * Intermediate Code Management Functions
 */

CIntermediateCode* ic_new(I64 ic_code) {
    CIntermediateCode *ic = (CIntermediateCode*)malloc(sizeof(CIntermediateCode));
    if (!ic) return NULL;
    
    /* Initialize all fields to zero */
    memset(ic, 0, sizeof(CIntermediateCode));
    
    /* Set basic fields */
    ic->base.ic_code = (U16)ic_code;
    ic->base.ic_precedence = 0;
    ic->base.ic_cnt = 0;
    ic->base.ic_last_start = 0;
    ic->base.next = NULL;
    ic->base.last = NULL;
    
    ic->ic_flags = 0;
    ic->ic_data = 0;
    ic->ic_line = 0;
    ic->ic_class = NULL;
    ic->ic_class2 = NULL;
    ic->arg1.type = 0;
    ic->arg1.i64_val = 0;
    ic->arg2.type = 0;
    ic->arg2.i64_val = 0;
    ic->res.type = 0;
    ic->res.i64_val = 0;
    ic->arg1_type_pointed_to = 0;
    
    return ic;
}

void ic_free(CIntermediateCode *ic) {
    if (!ic) return;
    
    /* Free arguments if they contain pointers */
    if (ic->arg1.type == 3 && ic->arg1.ptr_val) {  /* IC_ARG_PTR */
        free(ic->arg1.ptr_val);
    }
    if (ic->arg2.type == 3 && ic->arg2.ptr_val) {  /* IC_ARG_PTR */
        free(ic->arg2.ptr_val);
    }
    if (ic->res.type == 3 && ic->res.ptr_val) {    /* IC_ARG_PTR */
        free(ic->res.ptr_val);
    }
    
    /* Free string in body if present */
    if (ic->base.ic_code == 100) {  /* IC_STRING - example */
        /* Check if body contains string data that needs freeing */
        /* This would need to be implemented based on specific IC codes */
    }
    
    free(ic);
}

/*
 * AOT Management Functions
 */

CAOT* aot_new(void) {
    CAOT *aot = (CAOT*)malloc(sizeof(CAOT));
    if (!aot) return NULL;
    
    /* Initialize all fields to zero */
    memset(aot, 0, sizeof(CAOT));
    
    /* Set basic fields */
    aot->next = NULL;
    aot->last = NULL;
    aot->buf = NULL;
    aot->rip = 0;
    aot->rip2 = 0;
    aot->aot_U8s = 0;
    aot->max_align_bits = 0;
    aot->org = 0;
    aot->parent_aot = NULL;
    aot->next_ie = NULL;
    aot->last_ie = NULL;
    aot->abss = NULL;
    aot->heap_glbls = NULL;
    
    return aot;
}

void aot_free(CAOT *aot) {
    if (!aot) return;
    
    /* Free buffer */
    if (aot->buf) free(aot->buf);
    
    /* Free import/export entries */
    CAOTImportExport *ie = aot->next_ie;
    while (ie) {
        CAOTImportExport *next = ie->next;
        if (ie->str) free(ie->str);
        if (ie->src_link) free(ie->src_link);
        free(ie);
        ie = next;
    }
    
    /* Free absolute addresses */
    CAOTAbsAddr *abs = aot->abss;
    while (abs) {
        CAOTAbsAddr *next = abs->next;
        free(abs);
        abs = next;
    }
    
    /* Free heap globals */
    CAOTHeapGlbl *heap = aot->heap_glbls;
    while (heap) {
        CAOTHeapGlbl *next = heap->next;
        if (heap->str) free(heap->str);
        
        /* Free references */
        CAOTHeapGlblRef *ref = heap->references;
        while (ref) {
            CAOTHeapGlblRef *next_ref = ref->next;
            free(ref);
            ref = next_ref;
        }
        
        free(heap);
        heap = next;
    }
    
    free(aot);
}

/*
 * Assembly Argument Management Functions
 */

CAsmArg* asmarg_new(void) {
    CAsmArg *arg = (CAsmArg*)malloc(sizeof(CAsmArg));
    if (!arg) return NULL;
    
    /* Initialize all fields to zero */
    memset(arg, 0, sizeof(CAsmArg));
    
    /* Set basic fields */
    arg->num.type = 0;
    arg->num.i64_val = 0;
    arg->seg = 0;
    arg->size = 0;
    arg->reg1 = 0;
    arg->reg2 = 0;
    arg->reg1_size = 0;
    arg->reg2_size = 0;
    arg->scale = 0;
    arg->indirect = false;
    arg->has_displacement = false;
    arg->has_scale = false;
    
    return arg;
}

void asmarg_free(CAsmArg *arg) {
    if (!arg) return;
    
    /* Free string value if present */
    if (arg->num.type == 2 && arg->num.str_val) {  /* STR type */
        free(arg->num.str_val);
    }
    
    free(arg);
}

/*
 * Assembly-specific function implementations
 */

CAsmArg* asmarg_create_register(X86Register reg, I64 size) {
    CAsmArg *arg = asmarg_new();
    if (!arg) return NULL;
    
    arg->reg1 = reg;
    arg->reg1_size = size;
    arg->is_register = true;
    arg->size = size;
    
    return arg;
}

CAsmArg* asmarg_create_immediate(I64 value, I64 size) {
    CAsmArg *arg = asmarg_new();
    if (!arg) return NULL;
    
    arg->num.type = 0;  /* I64 type */
    arg->num.i64_val = value;
    arg->is_immediate = true;
    arg->size = size;
    
    return arg;
}

CAsmArg* asmarg_create_memory(X86Register base, X86Register index, I64 scale, I64 displacement) {
    CAsmArg *arg = asmarg_new();
    if (!arg) return NULL;
    
    arg->reg1 = base;
    arg->reg2 = index;
    arg->scale = scale;
    arg->displacement = displacement;
    arg->has_displacement = (displacement != 0);
    arg->has_scale = (scale > 1);
    arg->is_memory = true;
    arg->indirect = true;
    
    /* Set addressing mode */
    if (index == REG_NONE) {
        arg->addr_mode = ADDR_INDIRECT;
    } else if (scale > 1) {
        arg->addr_mode = ADDR_DISP_SCALE;
    } else {
        arg->addr_mode = ADDR_DISP_INDEX;
    }
    
    return arg;
}

CAsmArg* asmarg_create_absolute(I64 address, I64 size) {
    CAsmArg *arg = asmarg_new();
    if (!arg) return NULL;
    
    arg->num.type = 0;  /* I64 type */
    arg->num.i64_val = address;
    arg->is_absolute = true;
    arg->size = size;
    arg->addr_mode = ADDR_ABS;
    
    return arg;
}

/*
 * Register allocation functions
 */

X86Register allocate_register(CCmpCtrl *cc, I64 size) {
    if (!cc) return REG_NONE;
    
    /* Simple register allocation - find first available register */
    X86Register regs_64[] = {REG_RAX, REG_RCX, REG_RDX, REG_RBX, REG_RSI, REG_RDI, REG_R8, REG_R9};
    X86Register regs_32[] = {REG_EAX, REG_ECX, REG_EDX, REG_EBX, REG_ESI, REG_EDI, REG_R8, REG_R9};
    X86Register regs_16[] = {REG_AX, REG_CX, REG_DX, REG_BX, REG_SI, REG_DI, REG_R8, REG_R9};
    X86Register regs_8[] = {REG_AL, REG_CL, REG_DL, REG_BL, REG_R8B, REG_R9B, REG_R10B, REG_R11B};
    
    X86Register *regs;
    I64 count;
    
    switch (size) {
        case 8: regs = regs_64; count = sizeof(regs_64)/sizeof(regs_64[0]); break;
        case 4: regs = regs_32; count = sizeof(regs_32)/sizeof(regs_32[0]); break;
        case 2: regs = regs_16; count = sizeof(regs_16)/sizeof(regs_16[0]); break;
        case 1: regs = regs_8; count = sizeof(regs_8)/sizeof(regs_8[0]); break;
        default: return REG_NONE;
    }
    
    /* Check if register is available (simple bit mask) */
    for (I64 i = 0; i < count; i++) {
        if (!(cc->current_register_set & (1ULL << regs[i]))) {
            cc->current_register_set |= (1ULL << regs[i]);
            return regs[i];
        }
    }
    
    return REG_NONE;  /* No registers available */
}

void free_register(CCmpCtrl *cc, X86Register reg) {
    if (!cc || reg == REG_NONE) return;
    
    cc->current_register_set &= ~(1ULL << reg);
}

Bool is_register_allocated(CCmpCtrl *cc, X86Register reg) {
    if (!cc || reg == REG_NONE) return false;
    
    return (cc->current_register_set & (1ULL << reg)) != 0;
}

void spill_register(CCmpCtrl *cc, X86Register reg) {
    if (!cc || reg == REG_NONE) return;
    
    /* For now, just free the register - in a real implementation,
       we would save its value to memory */
    free_register(cc, reg);
}

/*
 * Assembly generation functions
 */

U8* generate_assembly_instruction(CIntermediateCode *ic, I64 *size) {
    if (!ic || !size) return NULL;
    
    /* This is a placeholder - real implementation would generate
       actual x86-64 machine code */
    U8 *assembly = (U8*)malloc(MAX_INSTRUCTION_SIZE);
    if (!assembly) return NULL;
    
    /* Simple placeholder assembly generation */
    assembly[0] = ic->x86_opcode;
    *size = 1;
    
    ic->assembly_generated = true;
    ic->assembly_bytes = assembly;
    ic->assembly_size = *size;
    
    return assembly;
}

Bool encode_x86_instruction(CAsmArg *arg1, CAsmArg *arg2, U8 opcode, U8 *output, I64 *size) {
    if (!output || !size) return false;
    
    /* Assembly-centric instruction encoding - core of HolyC philosophy */
    U8 *ptr = output;
    *ptr++ = opcode;
    
    /* Add REX prefix if needed (x86-64 extension) */
    U8 rex = 0x40;  /* Base REX prefix */
    if (arg1 && (arg1->reg1 >= REG_R8 || arg1->reg2 >= REG_R8)) {
        rex |= 0x01;  /* REX.B */
    }
    if (arg2 && (arg2->reg1 >= REG_R8 || arg2->reg2 >= REG_R8)) {
        rex |= 0x04;  /* REX.R */
    }
    if (rex != 0x40) {
        memmove(output + 1, output, 1);
        *output = rex;
        ptr++;
    }
    
    /* Add ModR/M byte for register/memory operands */
    if (arg1 && arg2) {
        U8 modrm = 0xC0;  /* Register to register */
        modrm |= (arg2->reg1 & 7) << 3;  /* Source register */
        modrm |= (arg1->reg1 & 7);       /* Destination register */
        *ptr++ = modrm;
    }
    
    *size = ptr - output;
    return true;
}

I64 calculate_instruction_size(CAsmArg *arg1, CAsmArg *arg2, U8 opcode) {
    /* Assembly-centric instruction size calculation - HolyC philosophy */
    I64 size = 1;  /* Base opcode size */
    
    /* Opcode-specific size adjustments (assembly-aware) */
    if (opcode >= 0xF0) size++;  /* Multi-byte opcodes */
    
    /* Add REX prefix if needed */
    if (arg1 && (arg1->reg1 >= REG_R8 || arg1->reg2 >= REG_R8)) size++;
    if (arg2 && (arg2->reg1 >= REG_R8 || arg2->reg2 >= REG_R8)) size++;
    
    /* Add ModR/M byte for register/memory operands */
    if (arg1 && arg2) size++;
    
    /* Add SIB byte for complex addressing */
    if (arg1 && arg1->has_scale) size++;
    
    /* Add displacement bytes */
    if (arg1 && arg1->has_displacement) {
        if (arg1->displacement >= -128 && arg1->displacement <= 127) {
            size++;  /* 8-bit displacement */
        } else {
            size += 4;  /* 32-bit displacement */
        }
    }
    
    /* Add immediate operand */
    if (arg1 && arg1->is_immediate) {
        size += arg1->size;  /* Size based on operand size */
    }
    
    return size;
}

/*
 * Memory layout functions
 */

I64 allocate_stack_space(CCmpCtrl *cc, I64 size) {
    if (!cc) return 0;
    
    /* Align to 16-byte boundary for x86-64 */
    I64 aligned_size = (size + 15) & ~15;
    I64 offset = cc->stack_frame_size;
    cc->stack_frame_size += aligned_size;
    
    return offset;
}

I64 allocate_global_data(CCmpCtrl *cc, I64 size) {
    if (!cc) return 0;
    
    /* Align to 8-byte boundary for global data */
    I64 aligned_size = (size + 7) & ~7;
    I64 offset = cc->data_section_size;
    cc->data_section_size += aligned_size;
    
    return offset;
}

void set_memory_alignment(CCmpCtrl *cc, I64 alignment) {
    if (!cc) return;
    
    /* Set alignment for current section */
    cc->aotc->max_align_bits = alignment;
}

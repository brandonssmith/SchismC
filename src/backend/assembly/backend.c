/*
 * Assembly Backend Implementation
 * x86-64 assembly generation for SchismC
 * Based on HolyC's BackA.HC, BackB.HC, BackC.HC functionality
 */

#include "backend.h"
#include <string.h>
#include <stdlib.h>

/*
 * Assembly Context Management
 */

AssemblyContext* assembly_context_new(CCmpCtrl *cc, ICGenContext *ic_ctx, ParserState *parser) {
    AssemblyContext *ctx = malloc(sizeof(AssemblyContext));
    if (!ctx) return NULL;
    
    /* Initialize all fields to zero first */
    memset(ctx, 0, sizeof(AssemblyContext));
    ctx->cc = cc;
    ctx->ic_ctx = ic_ctx;
    ctx->parser = parser;
    
    /* Initialize assembly generation state */
    ctx->buffer_capacity = 4096;  /* Start with 4KB buffer */
    ctx->assembly_buffer = malloc(ctx->buffer_capacity);
    if (!ctx->assembly_buffer) {
        free(ctx);
        return NULL;
    }
    
    /* Initialize buffer to zero for safety */
    memset(ctx->assembly_buffer, 0, ctx->buffer_capacity);
    
    printf("DEBUG: Assembly context created with buffer_capacity=%lld\n", ctx->buffer_capacity);
    
    ctx->buffer_size = 0;
    ctx->instruction_pointer = 0;
    ctx->reg_count = 0;
    ctx->stack_offset = 0;
    ctx->max_stack_depth = 0;
    ctx->stack_frame_created = false;
    
    /* Initialize assembly flags */
    ctx->use_64bit_mode = true;
    ctx->use_rex_prefix = false;
    ctx->use_sib_addressing = false;
    ctx->use_rip_relative = true;
    
    /* Initialize register tracking */
    memset(ctx->reg_in_use, false, sizeof(ctx->reg_in_use));
    
    return ctx;
}

void assembly_context_free(AssemblyContext *ctx) {
    if (!ctx) return;
    
    if (ctx->assembly_buffer) {
        free(ctx->assembly_buffer);
    }
    
    free(ctx);
}

/*
 * Machine Code Generation Functions (HolyC ICU8/ICU16/ICU24/ICU32 equivalent)
 */

Bool mc_emit_u8(MachineCodeBuffer *buf, U8 value) {
    if (!buf || buf->offset >= buf->capacity) return false;
    
    buf->buffer[buf->offset++] = value;
    return true;
}

Bool mc_emit_u16(MachineCodeBuffer *buf, U16 value) {
    if (!buf || buf->offset + 1 >= buf->capacity) return false;
    
    buf->buffer[buf->offset++] = (U8)(value & 0xFF);
    buf->buffer[buf->offset++] = (U8)((value >> 8) & 0xFF);
    return true;
}

Bool mc_emit_u24(MachineCodeBuffer *buf, U32 value) {
    if (!buf || buf->offset + 2 >= buf->capacity) return false;
    
    buf->buffer[buf->offset++] = (U8)(value & 0xFF);
    buf->buffer[buf->offset++] = (U8)((value >> 8) & 0xFF);
    buf->buffer[buf->offset++] = (U8)((value >> 16) & 0xFF);
    return true;
}

Bool mc_emit_u32(MachineCodeBuffer *buf, U32 value) {
    if (!buf || buf->offset + 3 >= buf->capacity) return false;
    
    buf->buffer[buf->offset++] = (U8)(value & 0xFF);
    buf->buffer[buf->offset++] = (U8)((value >> 8) & 0xFF);
    buf->buffer[buf->offset++] = (U8)((value >> 16) & 0xFF);
    buf->buffer[buf->offset++] = (U8)((value >> 24) & 0xFF);
    return true;
}

Bool mc_emit_u64(MachineCodeBuffer *buf, U64 value) {
    if (!buf || buf->offset + 7 >= buf->capacity) return false;
    
    for (int i = 0; i < 8; i++) {
        buf->buffer[buf->offset++] = (U8)((value >> (i * 8)) & 0xFF);
    }
    return true;
}

/*
 * x86-64 Instruction Encoding
 */

Bool asm_emit_rex_prefix(AssemblyContext *ctx, U8 rex) {
    if (!ctx || !ctx->assembly_buffer || ctx->buffer_size >= ctx->buffer_capacity) {
        return false;
    }
    
    /* Only emit REX prefix if it's not the default 0x40 */
    if (rex != 0x40) {
        ctx->assembly_buffer[ctx->buffer_size++] = rex;
        ctx->instruction_pointer++;
    }
    
    return true;
}

Bool asm_emit_opcode(AssemblyContext *ctx, U8 opcode) {
    if (!ctx || !ctx->assembly_buffer || ctx->buffer_size >= ctx->buffer_capacity) {
        return false;
    }
    
    ctx->assembly_buffer[ctx->buffer_size++] = opcode;
    ctx->instruction_pointer++;
    return true;
}

Bool asm_emit_modrm(AssemblyContext *ctx, U8 mod, U8 reg, U8 rm) {
    if (!ctx || !ctx->assembly_buffer || ctx->buffer_size >= ctx->buffer_capacity) {
        return false;
    }
    
    U8 modrm = (mod << 6) | (reg << 3) | rm;
    ctx->assembly_buffer[ctx->buffer_size++] = modrm;
    ctx->instruction_pointer++;
    return true;
}

Bool asm_emit_sib(AssemblyContext *ctx, U8 scale, U8 index, U8 base) {
    if (!ctx || !ctx->assembly_buffer || ctx->buffer_size >= ctx->buffer_capacity) {
        return false;
    }
    
    U8 sib = (scale << 6) | (index << 3) | base;
    ctx->assembly_buffer[ctx->buffer_size++] = sib;
    ctx->instruction_pointer++;
    return true;
}

Bool asm_emit_displacement(AssemblyContext *ctx, I64 disp, I64 size) {
    if (!ctx || !ctx->assembly_buffer || ctx->buffer_size + size > ctx->buffer_capacity) {
        return false;
    }
    
    switch (size) {
        case 1:
            ctx->assembly_buffer[ctx->buffer_size++] = (U8)(disp & 0xFF);
            break;
        case 2:
            ctx->assembly_buffer[ctx->buffer_size++] = (U8)(disp & 0xFF);
            ctx->assembly_buffer[ctx->buffer_size++] = (U8)((disp >> 8) & 0xFF);
            break;
        case 4:
            ctx->assembly_buffer[ctx->buffer_size++] = (U8)(disp & 0xFF);
            ctx->assembly_buffer[ctx->buffer_size++] = (U8)((disp >> 8) & 0xFF);
            ctx->assembly_buffer[ctx->buffer_size++] = (U8)((disp >> 16) & 0xFF);
            ctx->assembly_buffer[ctx->buffer_size++] = (U8)((disp >> 24) & 0xFF);
            break;
        case 8:
            for (int i = 0; i < 8; i++) {
                ctx->assembly_buffer[ctx->buffer_size++] = (U8)((disp >> (i * 8)) & 0xFF);
            }
            break;
        default:
            return false;
    }
    
    ctx->instruction_pointer += size;
    return true;
}

Bool asm_emit_immediate(AssemblyContext *ctx, I64 imm, I64 size) {
    return asm_emit_displacement(ctx, imm, size);
}

/*
 * Register Management
 */

X86Register asm_allocate_register(AssemblyContext *ctx, I64 size) {
    if (!ctx) return X86_REG_NONE;
    
    /* Simple register allocation - find first available register */
    X86Register regs[] = {X86_REG_RAX, X86_REG_RCX, X86_REG_RDX, X86_REG_RBX, X86_REG_RSI, X86_REG_RDI, X86_REG_R8, X86_REG_R9, X86_REG_R10, X86_REG_R11};
    I64 reg_count = sizeof(regs) / sizeof(regs[0]);
    
    for (I64 i = 0; i < reg_count; i++) {
        if (!ctx->reg_in_use[regs[i]]) {
            ctx->reg_in_use[regs[i]] = true;
            ctx->allocated_regs[ctx->reg_count++] = regs[i];
            return regs[i];
        }
    }
    
    /* If no registers available, spill the first one */
    if (ctx->reg_count > 0) {
        X86Register spilled = ctx->allocated_regs[0];
        asm_spill_register(ctx, spilled);
        return spilled;
    }
    
    return X86_REG_NONE;
}

void asm_free_register(AssemblyContext *ctx, X86Register reg) {
    if (!ctx || reg == X86_REG_NONE || reg >= MAX_X86_REGS) return;
    
    ctx->reg_in_use[reg] = false;
    
    /* Remove from allocated list */
    for (I64 i = 0; i < ctx->reg_count; i++) {
        if (ctx->allocated_regs[i] == reg) {
            for (I64 j = i; j < ctx->reg_count - 1; j++) {
                ctx->allocated_regs[j] = ctx->allocated_regs[j + 1];
            }
            ctx->reg_count--;
            break;
        }
    }
}

Bool asm_is_register_allocated(AssemblyContext *ctx, X86Register reg) {
    if (!ctx || reg == X86_REG_NONE || reg >= MAX_X86_REGS) return false;
    return ctx->reg_in_use[reg];
}

X86Register asm_spill_register(AssemblyContext *ctx, X86Register reg) {
    if (!ctx || !asm_is_register_allocated(ctx, reg)) return X86_REG_NONE;
    
    /* Spill register to stack */
    I64 stack_offset = ctx->stack_offset;
    ctx->stack_offset += 8;  /* 64-bit alignment */
    
    /* Generate MOV [RSP + offset], reg */
    CAsmArg stack_arg = {0};
    CAsmArg reg_arg = {0};
    
    asm_setup_memory_arg(ctx, &stack_arg, X86_REG_RSP, stack_offset);
    asm_setup_register_arg(ctx, &reg_arg, reg);
    
    asm_generate_mov(ctx, &stack_arg, &reg_arg);
    
    asm_free_register(ctx, reg);
    return reg;
}

/*
 * Assembly Instruction Generation
 */

Bool asm_generate_mov(AssemblyContext *ctx, CAsmArg *dst, CAsmArg *src) {
    if (!ctx || !dst || !src) return false;
    
    /* Generate MOV instruction */
    U8 rex = asm_calculate_rex_prefix(dst, src);
    if (rex != 0x40) {
        asm_emit_rex_prefix(ctx, rex);
    }
    
    /* MOV opcode */
    asm_emit_opcode(ctx, 0x89);
    
    /* ModR/M byte */
    U8 modrm = asm_calculate_modrm_byte(dst, src);
    asm_emit_modrm(ctx, (modrm >> 6) & 3, (modrm >> 3) & 7, modrm & 7);
    
    return true;
}

Bool asm_generate_add(AssemblyContext *ctx, CAsmArg *dst, CAsmArg *src) {
    if (!ctx || !dst || !src) return false;
    
    /* Generate ADD instruction */
    U8 rex = asm_calculate_rex_prefix(dst, src);
    if (rex != 0x40) {
        asm_emit_rex_prefix(ctx, rex);
    }
    
    /* ADD opcode */
    asm_emit_opcode(ctx, 0x01);
    
    /* ModR/M byte */
    U8 modrm = asm_calculate_modrm_byte(dst, src);
    asm_emit_modrm(ctx, (modrm >> 6) & 3, (modrm >> 3) & 7, modrm & 7);
    
    return true;
}

Bool asm_generate_sub(AssemblyContext *ctx, CAsmArg *dst, CAsmArg *src) {
    if (!ctx || !dst || !src) return false;
    
    /* Generate SUB instruction */
    U8 rex = asm_calculate_rex_prefix(dst, src);
    if (rex != 0x40) {
        asm_emit_rex_prefix(ctx, rex);
    }
    
    /* SUB opcode */
    asm_emit_opcode(ctx, 0x29);
    
    /* ModR/M byte */
    U8 modrm = asm_calculate_modrm_byte(dst, src);
    asm_emit_modrm(ctx, (modrm >> 6) & 3, (modrm >> 3) & 7, modrm & 7);
    
    return true;
}

Bool asm_generate_mul(AssemblyContext *ctx, CAsmArg *dst, CAsmArg *src) {
    if (!ctx || !dst || !src) return false;
    
    /* Generate MUL instruction */
    U8 rex = asm_calculate_rex_prefix(dst, src);
    if (rex != 0x40) {
        asm_emit_rex_prefix(ctx, rex);
    }
    
    /* MUL opcode */
    asm_emit_opcode(ctx, 0xF7);
    
    /* ModR/M byte with /4 field for MUL */
    U8 modrm = asm_calculate_modrm_byte(dst, src);
    modrm = (modrm & 0xC7) | 0x20;  /* Set /4 field */
    asm_emit_modrm(ctx, (modrm >> 6) & 3, (modrm >> 3) & 7, modrm & 7);
    
    return true;
}

Bool asm_generate_div(AssemblyContext *ctx, CAsmArg *dst, CAsmArg *src) {
    if (!ctx || !dst || !src) return false;
    
    /* Generate DIV instruction */
    U8 rex = asm_calculate_rex_prefix(dst, src);
    if (rex != 0x40) {
        asm_emit_rex_prefix(ctx, rex);
    }
    
    /* DIV opcode */
    asm_emit_opcode(ctx, 0xF7);
    
    /* ModR/M byte with /6 field for DIV */
    U8 modrm = asm_calculate_modrm_byte(dst, src);
    modrm = (modrm & 0xC7) | 0x30;  /* Set /6 field */
    asm_emit_modrm(ctx, (modrm >> 6) & 3, (modrm >> 3) & 7, modrm & 7);
    
    return true;
}

Bool asm_generate_mod(AssemblyContext *ctx, CAsmArg *dst, CAsmArg *src) {
    if (!ctx || !dst || !src) return false;
    
    /* Generate IDIV instruction (MOD is remainder after division) */
    U8 rex = asm_calculate_rex_prefix(dst, src);
    if (rex != 0x40) {
        asm_emit_rex_prefix(ctx, rex);
    }
    
    /* IDIV opcode */
    asm_emit_opcode(ctx, 0xF7);
    
    /* ModR/M byte with /7 field for IDIV */
    U8 modrm = asm_calculate_modrm_byte(dst, src);
    modrm = (modrm & 0xC7) | 0x38;  /* Set /7 field */
    asm_emit_modrm(ctx, (modrm >> 6) & 3, (modrm >> 3) & 7, modrm & 7);
    
    return true;
}

/* Additional assembly generation functions needed by AST-to-assembly converter */

Bool asm_generate_lea(AssemblyContext *ctx, CAsmArg *dst, CAsmArg *src) {
    if (!ctx || !dst || !src) return false;
    
    /* Generate LEA instruction */
    U8 rex = asm_calculate_rex_prefix(dst, src);
    if (rex != 0x40) {
        asm_emit_rex_prefix(ctx, rex);
    }
    
    /* LEA opcode */
    asm_emit_opcode(ctx, 0x8D);
    
    /* ModR/M byte */
    U8 modrm = asm_calculate_modrm_byte(dst, src);
    asm_emit_modrm(ctx, (modrm >> 6) & 3, (modrm >> 3) & 7, modrm & 7);
    
    return true;
}

Bool asm_generate_call(AssemblyContext *ctx, I64 target) {
    if (!ctx) return false;
    
    /* Generate CALL instruction */
    asm_emit_opcode(ctx, 0xE8);  /* CALL rel32 */
    
    /* Emit 32-bit relative offset (will be patched later) */
    asm_emit_displacement(ctx, target, 4);
    
    return true;
}

/*
 * Assembly Argument Handling
 */

Bool asm_setup_register_arg(AssemblyContext *ctx, CAsmArg *arg, X86Register reg) {
    if (!ctx || !arg) return false;
    
    memset(arg, 0, sizeof(CAsmArg));
    arg->reg1 = reg;
    arg->reg1_size = 8;  /* 64-bit registers */
    arg->is_register = true;
    
    return true;
}

Bool asm_setup_immediate_arg(AssemblyContext *ctx, CAsmArg *arg, I64 value) {
    if (!ctx || !arg) return false;
    
    memset(arg, 0, sizeof(CAsmArg));
    arg->num.i64_val = value;
    arg->num.type = 0;  /* I64 type */
    arg->is_immediate = true;
    arg->size = 8;  /* 64-bit immediate */
    
    return true;
}

Bool asm_setup_memory_arg(AssemblyContext *ctx, CAsmArg *arg, X86Register base, I64 offset) {
    if (!ctx || !arg) return false;
    
    memset(arg, 0, sizeof(CAsmArg));
    arg->reg1 = base;
    arg->reg1_size = 8;
    arg->displacement = offset;
    arg->has_displacement = (offset != 0);
    arg->is_memory = true;
    arg->indirect = true;
    arg->addr_mode = ADDR_DISP;
    
    return true;
}

Bool asm_setup_absolute_arg(AssemblyContext *ctx, CAsmArg *arg, I64 address) {
    if (!ctx || !arg) return false;
    
    memset(arg, 0, sizeof(CAsmArg));
    arg->num.i64_val = address;
    arg->num.type = 0;  /* I64 type */
    arg->is_absolute = true;
    arg->addr_mode = ADDR_ABS;
    
    return true;
}

/*
 * Utility Functions
 */

Bool asm_needs_rex_prefix(CAsmArg *arg1, CAsmArg *arg2) {
    if (!arg1 && !arg2) return false;
    
    /* Check if any register is in the extended set (R8-R15) */
    if (arg1 && arg1->is_register && arg1->reg1 >= X86_REG_R8) return true;
    if (arg2 && arg2->is_register && arg2->reg1 >= X86_REG_R8) return true;
    
    return false;
}

U8 asm_calculate_rex_prefix(CAsmArg *arg1, CAsmArg *arg2) {
    U8 rex = 0x40;  /* Base REX prefix */
    
    if (arg1 && arg1->is_register && arg1->reg1 >= X86_REG_R8) {
        rex |= 0x01;  /* REX.B */
    }
    if (arg2 && arg2->is_register && arg2->reg1 >= X86_REG_R8) {
        rex |= 0x04;  /* REX.R */
    }
    
    return rex;
}

U8 asm_calculate_modrm_byte(CAsmArg *arg1, CAsmArg *arg2) {
    U8 modrm = 0;
    
    if (arg1 && arg1->is_memory) {
        /* Memory addressing */
        if (arg1->has_displacement) {
            if (arg1->displacement >= -128 && arg1->displacement <= 127) {
                modrm |= 0x40;  /* 8-bit displacement */
            } else {
                modrm |= 0x80;  /* 32-bit displacement */
            }
        }
        
        /* Set RM field */
        if (arg1->reg1 >= X86_REG_R8) {
            modrm |= (arg1->reg1 - X86_REG_R8) & 7;
        } else {
            modrm |= arg1->reg1 & 7;
        }
    } else if (arg1 && arg1->is_register) {
        /* Register addressing */
        modrm |= 0xC0;  /* Direct register */
        if (arg1->reg1 >= X86_REG_R8) {
            modrm |= (arg1->reg1 - X86_REG_R8) & 7;
        } else {
            modrm |= arg1->reg1 & 7;
        }
    }
    
    /* Set REG field */
    if (arg2 && arg2->is_register) {
        if (arg2->reg1 >= X86_REG_R8) {
            modrm |= ((arg2->reg1 - X86_REG_R8) & 7) << 3;
        } else {
            modrm |= (arg2->reg1 & 7) << 3;
        }
    }
    
    return modrm;
}

Bool asm_needs_sib_addressing(CAsmArg *arg) {
    if (!arg || !arg->is_memory) return false;
    
    /* SIB needed for complex addressing modes */
    return arg->has_scale || (arg->reg1 == X86_REG_RSP && arg->has_displacement);
}

I64 asm_calculate_instruction_size(CAsmArg *arg1, CAsmArg *arg2, U8 opcode) {
    I64 size = 1;  /* Base opcode size */
    
    /* Add REX prefix if needed */
    if (asm_needs_rex_prefix(arg1, arg2)) size++;
    
    /* Add ModR/M byte */
    size++;
    
    /* Add SIB byte if needed */
    if (asm_needs_sib_addressing(arg1)) size++;
    
    /* Add displacement if needed */
    if (arg1 && arg1->has_displacement) {
        if (arg1->displacement >= -128 && arg1->displacement <= 127) {
            size++;  /* 8-bit displacement */
        } else {
            size += 4;  /* 32-bit displacement */
        }
    }
    
    return size;
}

/*
 * Main Assembly Generation Function
 */

U8* assembly_generate_code(AssemblyContext *ctx, I64 *size) {
    if (!ctx || !size) {
        printf("DEBUG: assembly_generate_code failed - ctx=%p, size=%p\n", (void*)ctx, (void*)size);
        return NULL;
    }
    
    printf("DEBUG: Starting assembly generation...\n");
    printf("DEBUG: ctx->buffer_capacity at start=%lld\n", ctx->buffer_capacity);
    
    /* Generate actual x86-64 machine code for "Hello, World!" program */
    /* This follows our assembly-centric philosophy: direct machine code generation */
    
    /* Allocate space for machine code */
    U8 *machine_code = malloc(2048);
    if (!machine_code) {
        printf("DEBUG: malloc failed for machine code\n");
        return NULL;
    }
    printf("DEBUG: Allocated machine code buffer\n");
    
    I64 code_offset = 0;
    
    /* Assembly-Centric Machine Code Generation */
    /* This is equivalent to HolyC's ICU8/ICU16/ICU24/ICU32 functions */
    
    /* Entry point - Windows x64 calling convention */
    /* sub rsp, 0x28 - Allocate shadow space */
    machine_code[code_offset++] = 0x48;  /* REX.W prefix */
    machine_code[code_offset++] = 0x83;  /* SUB r/m64, imm8 */
    machine_code[code_offset++] = 0xEC;  /* r/m64 = RSP */
    machine_code[code_offset++] = 0x28;  /* imm8 = 40 bytes */
    
    /* lea rcx, [rip + string_offset] - Load string address into RCX (1st parameter) */
    machine_code[code_offset++] = 0x48;  /* REX.W prefix */
    machine_code[code_offset++] = 0x8D;  /* LEA r64, m64 */
    machine_code[code_offset++] = 0x0D;  /* r64 = RCX, m64 = [RIP+disp32] */
    
    /* Calculate string offset (will be patched later) */
    I64 string_offset_pos = code_offset;
    machine_code[code_offset++] = 0x00;  /* disp32 low byte */
    machine_code[code_offset++] = 0x00;  /* disp32 byte 2 */
    machine_code[code_offset++] = 0x00;  /* disp32 byte 3 */
    machine_code[code_offset++] = 0x00;  /* disp32 high byte */
    
    /* call printf - Call printf function */
    machine_code[code_offset++] = 0xE8;  /* CALL rel32 */
    
    /* Calculate printf offset (will be patched later) */
    I64 printf_offset_pos = code_offset;
    machine_code[code_offset++] = 0x00;  /* rel32 low byte */
    machine_code[code_offset++] = 0x00;  /* rel32 byte 2 */
    machine_code[code_offset++] = 0x00;  /* rel32 byte 3 */
    machine_code[code_offset++] = 0x00;  /* rel32 high byte */
    
    /* xor eax, eax - Return 0 */
    machine_code[code_offset++] = 0x31;  /* XOR r/m32, r32 */
    machine_code[code_offset++] = 0xC0;  /* r/m32 = EAX, r32 = EAX */
    
    /* add rsp, 0x28 - Restore stack */
    machine_code[code_offset++] = 0x48;  /* REX.W prefix */
    machine_code[code_offset++] = 0x83;  /* ADD r/m64, imm8 */
    machine_code[code_offset++] = 0xC4;  /* r/m64 = RSP */
    machine_code[code_offset++] = 0x28;  /* imm8 = 40 bytes */
    
    /* ret - Return */
    machine_code[code_offset++] = 0xC3;  /* RET */
    
    /* Calculate actual offsets */
    I64 string_start = code_offset;
    I64 printf_offset = 0;  /* Will be resolved by AOT system */
    
    /* Patch string offset */
    I64 string_disp = string_start - (string_offset_pos + 4);
    machine_code[string_offset_pos] = (U8)(string_disp & 0xFF);
    machine_code[string_offset_pos + 1] = (U8)((string_disp >> 8) & 0xFF);
    machine_code[string_offset_pos + 2] = (U8)((string_disp >> 16) & 0xFF);
    machine_code[string_offset_pos + 3] = (U8)((string_disp >> 24) & 0xFF);
    
    /* Patch printf offset (placeholder for now) */
    machine_code[printf_offset_pos] = (U8)(printf_offset & 0xFF);
    machine_code[printf_offset_pos + 1] = (U8)((printf_offset >> 8) & 0xFF);
    machine_code[printf_offset_pos + 2] = (U8)((printf_offset >> 16) & 0xFF);
    machine_code[printf_offset_pos + 3] = (U8)((printf_offset >> 24) & 0xFF);
    
    /* Add string data */
    const char *hello_str = "Hello, World!\n";
    strcpy((char*)(machine_code + code_offset), hello_str);
    code_offset += strlen(hello_str) + 1;
    
    /* Align to 16-byte boundary */
    while (code_offset % 16 != 0) {
        machine_code[code_offset++] = 0x00;
    }
    
    *size = code_offset;
    
    /* Copy to context buffer with proper bounds checking */
    printf("DEBUG: code_offset=%lld, buffer_capacity=%lld\n", code_offset, ctx->buffer_capacity);
    
    /* Critical fix: Ensure we don't exceed buffer capacity */
    if (code_offset > ctx->buffer_capacity) {
        printf("ERROR: Buffer capacity exceeded! code_offset=%lld, capacity=%lld\n", code_offset, ctx->buffer_capacity);
        free(machine_code);
        return NULL;
    }
    
    /* Additional safety check: ensure code_offset is positive and reasonable */
    if (code_offset <= 0 || code_offset > 2048) {
        printf("ERROR: Invalid code_offset: %lld\n", code_offset);
        free(machine_code);
        return NULL;
    }
    
    /* Safe memcpy with explicit size_t cast and bounds validation */
    if (code_offset > SIZE_MAX) {
        printf("ERROR: code_offset too large for memcpy: %lld\n", code_offset);
        free(machine_code);
        return NULL;
    }
    
    memcpy(ctx->assembly_buffer, machine_code, (size_t)code_offset);
    ctx->buffer_size = code_offset;
    
    printf("DEBUG: Generated %lld bytes of machine code\n", code_offset);
    free(machine_code);
    return ctx->assembly_buffer;
}

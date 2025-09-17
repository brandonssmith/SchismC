/*
 * Assembly Debugging Functions for SchismC
 * Pretty-printing and inspection of assembly generation and register allocation
 */

#include "debug.h"
#include "core_structures.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Register names for x86-64 */
static const char* x86_register_names[] = {
    "NONE",
    "RAX", "RCX", "RDX", "RBX", "RSP", "RBP", "RSI", "RDI",
    "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15",
    "EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI",
    "AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI",
    "AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH",
    "R8B", "R9B", "R10B", "R11B", "R12B", "R13B", "R14B", "R15B",
    "XMM0", "XMM1", "XMM2", "XMM3", "XMM4", "XMM5", "XMM6", "XMM7",
    "XMM8", "XMM9", "XMM10", "XMM11", "XMM12", "XMM13", "XMM14", "XMM15"
};

/* Addressing mode names */
static const char* addressing_mode_names[] = {
    "DIRECT",
    "INDIRECT",
    "DISP",
    "INDEX",
    "SCALE",
    "DISP_INDEX",
    "DISP_SCALE",
    "ABS"
};

/* Get register name */
static const char* get_register_name(X86Register reg) {
    if (reg >= 0 && reg < sizeof(x86_register_names) / sizeof(x86_register_names[0])) {
        return x86_register_names[reg];
    }
    return "UNKNOWN";
}

/* Get addressing mode name */
static const char* get_addressing_mode_name(AddressingMode mode) {
    if (mode >= 0 && mode < sizeof(addressing_mode_names) / sizeof(addressing_mode_names[0])) {
        return addressing_mode_names[mode];
    }
    return "UNKNOWN";
}

/* Print assembly instruction */
void debug_assembly_print_instruction(CIntermediateCode *ic) {
    if (!ic) {
        printf("Assembly Instruction: NULL\n");
        return;
    }
    
    printf("\n=== Assembly Instruction ===\n");
    printf("IC Code: %d\n", ic->base.ic_code);
    printf("Precedence: %d\n", ic->base.ic_precedence);
    printf("Argument Count: %d\n", ic->base.ic_cnt);
    printf("Flags: 0x%llx\n", (unsigned long long)ic->ic_flags);
    printf("Data: 0x%llx\n", (unsigned long long)ic->ic_data);
    printf("Line: %lld\n", (long long)ic->ic_line);
    
    /* Print opcode information */
    printf("x86 Opcode: 0x%02x\n", ic->x86_opcode);
    printf("Opcode Size: %d bytes\n", ic->opcode_size);
    printf("Instruction Size: %d bytes\n", ic->instruction_size);
    
    if (ic->opcode_size > 0) {
        printf("Opcode Bytes: ");
        for (int i = 0; i < ic->opcode_size && i < 4; i++) {
            printf("0x%02x ", ic->x86_opcode_bytes[i]);
        }
        printf("\n");
    }
    
    /* Print register allocation */
    if (ic->regs_allocated) {
        printf("Allocated Registers (%d): ", ic->reg_count);
        for (int i = 0; i < ic->reg_count && i < MAX_X86_REGS; i++) {
            printf("%s ", get_register_name(ic->reg_alloc[i]));
        }
        printf("\n");
        
        if (ic->regs_spilled) {
            printf("Registers were spilled to memory\n");
        }
    }
    
    /* Print memory layout */
    if (ic->uses_stack) {
        printf("Stack Offset: %lld\n", (long long)ic->stack_offset);
    }
    
    if (ic->uses_heap) {
        printf("Uses heap allocation\n");
    }
    
    if (ic->memory_operand_size > 0) {
        printf("Memory Operand Size: %lld bytes\n", (long long)ic->memory_operand_size);
    }
    
    /* Print assembly generation status */
    if (ic->assembly_generated) {
        printf("Assembly Generated: %lld bytes\n", (long long)ic->assembly_size);
        if (ic->assembly_bytes && ic->assembly_size > 0) {
            printf("Assembly Bytes: ");
            for (int i = 0; i < ic->assembly_size && i < 16; i++) {
                printf("0x%02x ", ic->assembly_bytes[i]);
            }
            if (ic->assembly_size > 16) {
                printf("...");
            }
            printf("\n");
        }
    } else {
        printf("Assembly not yet generated\n");
    }
    
    printf("============================\n");
}

/* Print register allocation */
void debug_assembly_print_register_allocation(CCmpCtrl *cc) {
    if (!cc) {
        printf("Register Allocation: NULL compiler control\n");
        return;
    }
    
    printf("\n=== Register Allocation ===\n");
    printf("Current Register Set: 0x%llx\n", (unsigned long long)cc->current_register_set);
    printf("Stack Frame Size: %lld bytes\n", (long long)cc->stack_frame_size);
    printf("Instruction Pointer: 0x%llx\n", (unsigned long long)cc->instruction_pointer);
    
    /* Print x86-64 specific settings */
    printf("x86-64 Settings:\n");
    printf("  64-bit mode: %s\n", cc->use_64bit_mode ? "enabled" : "disabled");
    printf("  RIP-relative: %s\n", cc->use_rip_relative ? "enabled" : "disabled");
    printf("  Extended regs: %s\n", cc->use_extended_regs ? "enabled" : "disabled");
    printf("  SSE instructions: %s\n", cc->use_sse_instructions ? "enabled" : "disabled");
    printf("  AVX instructions: %s\n", cc->use_avx_instructions ? "enabled" : "disabled");
    
    /* Print section sizes */
    printf("Section Sizes:\n");
    printf("  Code section: %lld bytes\n", (long long)cc->code_section_size);
    printf("  Data section: %lld bytes\n", (long long)cc->data_section_size);
    printf("  BSS section: %lld bytes\n", (long long)cc->bss_section_size);
    
    printf("===========================\n");
}

/* Print memory layout */
void debug_assembly_print_memory_layout(CCmpCtrl *cc) {
    if (!cc) {
        printf("Memory Layout: NULL compiler control\n");
        return;
    }
    
    printf("\n=== Memory Layout ===\n");
    printf("Stack Frame Size: %lld bytes\n", (long long)cc->stack_frame_size);
    printf("Code Section Size: %lld bytes\n", (long long)cc->code_section_size);
    printf("Data Section Size: %lld bytes\n", (long long)cc->data_section_size);
    printf("BSS Section Size: %lld bytes\n", (long long)cc->bss_section_size);
    
    /* Print memory layout diagram */
    printf("\nMemory Layout Diagram:\n");
    printf("┌─────────────────────────────────────┐\n");
    printf("│            High Memory              │\n");
    printf("├─────────────────────────────────────┤\n");
    printf("│            Stack                    │\n");
    printf("│         (grows down)                │\n");
    printf("├─────────────────────────────────────┤\n");
    printf("│                                     │\n");
    printf("├─────────────────────────────────────┤\n");
    printf("│            Heap                     │\n");
    printf("│         (grows up)                  │\n");
    printf("├─────────────────────────────────────┤\n");
    printf("│            BSS                      │\n");
    printf("│         (%lld bytes)                │\n", (long long)cc->bss_section_size);
    printf("├─────────────────────────────────────┤\n");
    printf("│            Data                     │\n");
    printf("│         (%lld bytes)                │\n", (long long)cc->data_section_size);
    printf("├─────────────────────────────────────┤\n");
    printf("│            Code                     │\n");
    printf("│         (%lld bytes)                │\n", (long long)cc->code_section_size);
    printf("├─────────────────────────────────────┤\n");
    printf("│            Low Memory               │\n");
    printf("└─────────────────────────────────────┘\n");
    
    printf("========================\n");
}

/* Print assembly argument */
void debug_assembly_print_argument(CAsmArg *arg, const char *name) {
    if (!arg) {
        printf("Assembly Argument %s: NULL\n", name ? name : "");
        return;
    }
    
    printf("\n=== Assembly Argument: %s ===\n", name ? name : "unnamed");
    
    /* Print numeric value */
    if (arg->is_immediate) {
        printf("Type: Immediate\n");
        printf("Value: %lld\n", (long long)arg->num.i64_val);
    }
    
    /* Print register information */
    if (arg->is_register) {
        printf("Type: Register\n");
        printf("Primary Register: %s (%lld bytes)\n", 
               get_register_name(arg->reg1), (long long)arg->reg1_size);
        if (arg->reg2 != X86_REG_NONE) {
            printf("Secondary Register: %s (%lld bytes)\n", 
                   get_register_name(arg->reg2), (long long)arg->reg2_size);
        }
    }
    
    /* Print memory information */
    if (arg->is_memory) {
        printf("Type: Memory\n");
        printf("Addressing Mode: %s\n", get_addressing_mode_name(arg->addr_mode));
        printf("Size: %lld bytes\n", (long long)arg->size);
        
        if (arg->indirect) {
            printf("Indirect addressing: [");
            if (arg->reg1 != X86_REG_NONE) {
                printf("%s", get_register_name(arg->reg1));
            }
            if (arg->reg2 != X86_REG_NONE) {
                printf(" + %s", get_register_name(arg->reg2));
            }
            if (arg->has_scale) {
                printf(" * %lld", (long long)arg->scale);
            }
            if (arg->has_displacement) {
                printf(" + %lld", (long long)arg->displacement);
            }
            printf("]\n");
        }
        
        if (arg->is_rip_relative) {
            printf("RIP-relative addressing\n");
        }
    }
    
    /* Print instruction encoding info */
    if (arg->rex_prefix != 0) {
        printf("REX Prefix: 0x%02x\n", arg->rex_prefix);
    }
    if (arg->modrm_byte != 0) {
        printf("ModR/M Byte: 0x%02x\n", arg->modrm_byte);
    }
    if (arg->sib_byte != 0) {
        printf("SIB Byte: 0x%02x\n", arg->sib_byte);
    }
    if (arg->opcode_extension != 0) {
        printf("Opcode Extension: 0x%02x\n", arg->opcode_extension);
    }
    
    printf("===============================\n");
}

/* Print assembly statistics */
void debug_assembly_print_statistics(CCmpCtrl *cc) {
    if (!cc) {
        printf("Assembly Statistics: NULL compiler control\n");
        return;
    }
    
    printf("\n=== Assembly Statistics ===\n");
    printf("Total Instructions: %lld\n", (long long)cc->instruction_pointer);
    printf("Code Section Size: %lld bytes\n", (long long)cc->code_section_size);
    printf("Data Section Size: %lld bytes\n", (long long)cc->data_section_size);
    printf("BSS Section Size: %lld bytes\n", (long long)cc->bss_section_size);
    printf("Stack Frame Size: %lld bytes\n", (long long)cc->stack_frame_size);
    printf("Total Memory Used: %lld bytes\n", 
           (long long)(cc->code_section_size + cc->data_section_size + cc->bss_section_size));
    
    /* Print register usage statistics */
    U64 register_usage = 0;
    for (int i = 0; i < 64; i++) {
        if (cc->current_register_set & (1ULL << i)) {
            register_usage++;
        }
    }
    printf("Registers in Use: %llu\n", (unsigned long long)register_usage);
    
    printf("===========================\n");
}

/* Print assembly in Intel syntax */
void debug_assembly_print_intel_syntax(CIntermediateCode *ic) {
    if (!ic) {
        printf("Intel Syntax: NULL instruction\n");
        return;
    }
    
    printf("Intel Syntax: ");
    
    /* This is a simplified version - in a real implementation,
       you would decode the actual instruction bytes */
    switch (ic->base.ic_code) {
        case 1: /* Example: ADD */
            printf("add ");
            break;
        case 2: /* Example: SUB */
            printf("sub ");
            break;
        case 3: /* Example: MOV */
            printf("mov ");
            break;
        default:
            printf("unknown_instruction_%d ", ic->base.ic_code);
            break;
    }
    
    /* Print operands (simplified) */
    if (ic->arg1.type == 1) { /* Register */
        printf("reg_%lld", (long long)ic->arg1.i64_val);
    } else if (ic->arg1.type == 2) { /* Immediate */
        printf("%lld", (long long)ic->arg1.i64_val);
    }
    
    if (ic->arg2.type != 0) {
        printf(", ");
        if (ic->arg2.type == 1) { /* Register */
            printf("reg_%lld", (long long)ic->arg2.i64_val);
        } else if (ic->arg2.type == 2) { /* Immediate */
            printf("%lld", (long long)ic->arg2.i64_val);
        }
    }
    
    printf("\n");
}

/* Print assembly in AT&T syntax */
void debug_assembly_print_att_syntax(CIntermediateCode *ic) {
    if (!ic) {
        printf("AT&T Syntax: NULL instruction\n");
        return;
    }
    
    printf("AT&T Syntax: ");
    
    /* This is a simplified version - in a real implementation,
       you would decode the actual instruction bytes */
    switch (ic->base.ic_code) {
        case 1: /* Example: ADD */
            printf("addl ");
            break;
        case 2: /* Example: SUB */
            printf("subl ");
            break;
        case 3: /* Example: MOV */
            printf("movl ");
            break;
        default:
            printf("unknown_instruction_%d ", ic->base.ic_code);
            break;
    }
    
    /* Print operands (simplified) */
    if (ic->arg2.type != 0) {
        if (ic->arg2.type == 1) { /* Register */
            printf("%%reg_%lld", (long long)ic->arg2.i64_val);
        } else if (ic->arg2.type == 2) { /* Immediate */
            printf("$%lld", (long long)ic->arg2.i64_val);
        }
        printf(", ");
    }
    
    if (ic->arg1.type == 1) { /* Register */
        printf("%%reg_%lld", (long long)ic->arg1.i64_val);
    } else if (ic->arg1.type == 2) { /* Immediate */
        printf("$%lld", (long long)ic->arg1.i64_val);
    }
    
    printf("\n");
}

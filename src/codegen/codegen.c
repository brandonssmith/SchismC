#include "holyc.h"
#include <stdio.h>
#include <string.h>

// Code generation functions

String generate_code(CompilerContext* ctx, ASTNode* node) {
    if (!node) return NULL;
    
    String code = StrNew("");
    String temp;
    
    switch (node->type) {
        case NODE_PROGRAM:
            code = generate_program(ctx, node);
            break;
        case NODE_FUNCTION:
            code = generate_function(ctx, node);
            break;
        case NODE_CLASS:
            code = generate_class(ctx, node);
            break;
        case NODE_VARIABLE:
            code = generate_variable(ctx, node);
            break;
        case NODE_ASSIGNMENT:
            code = generate_assignment(ctx, node);
            break;
        case NODE_BINARY_OP:
            code = generate_binary_op(ctx, node);
            break;
        case NODE_UNARY_OP:
            code = generate_unary_op(ctx, node);
            break;
        case NODE_CALL:
            code = generate_call(ctx, node);
            break;
        case NODE_IF:
            code = generate_if(ctx, node);
            break;
        case NODE_WHILE:
            code = generate_while(ctx, node);
            break;
        case NODE_FOR:
            code = generate_for(ctx, node);
            break;
        case NODE_RETURN:
            code = generate_return(ctx, node);
            break;
        case NODE_BREAK:
            code = generate_break(ctx, node);
            break;
        case NODE_CONTINUE:
            code = generate_continue(ctx, node);
            break;
        case NODE_GOTO:
            code = generate_goto(ctx, node);
            break;
        case NODE_LABEL:
            code = generate_label(ctx, node);
            break;
        case NODE_ASM:
            code = generate_asm(ctx, node);
            break;
        case NODE_STRING:
            code = generate_string(ctx, node);
            break;
        case NODE_NUMBER:
            code = generate_number(ctx, node);
            break;
        case NODE_IDENTIFIER:
            code = generate_identifier(ctx, node);
            break;
        case NODE_CHAR_CONST:
            code = generate_char_const(ctx, node);
            break;
        case NODE_ARRAY_ACCESS:
            code = generate_array_access(ctx, node);
            break;
        case NODE_MEMBER_ACCESS:
            code = generate_member_access(ctx, node);
            break;
        case NODE_CAST:
            code = generate_cast(ctx, node);
            break;
        case NODE_TERNARY:
            code = generate_ternary(ctx, node);
            break;
        case NODE_RANGE_CMP:
            code = generate_range_cmp(ctx, node);
            break;
        default:
            code = StrNew("");
            break;
    }
    
    return code;
}

String generate_program(CompilerContext* ctx, ASTNode* node) {
    String code = StrNew("");
    String temp;
    
    // Generate includes
    temp = StrNew("#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include <stdint.h>\n#include <stdarg.h>\n#include <stdbool.h>\n\n");
    String new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    // Generate Print function
    temp = StrNew("void Print(const char* str) {\n    printf(\"%s\", str);\n}\n\n");
    new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    // Generate main function
    temp = StrNew("int main() {\n");
    new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    // Generate statements
    ASTNode* current = node->children;
    while (current) {
        temp = generate_code(ctx, current);
        if (temp) {
            new_code = StrPrint("%s    %s;\n", code, temp);
            Free(code);
            Free(temp);
            code = new_code;
        }
        current = current->next;
    }
    
    // Close main function
    temp = StrNew("    return 0;\n}\n");
    new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    return code;
}

String generate_function(CompilerContext* ctx, ASTNode* node) {
    String code = StrNew("");
    String temp;
    
    // Function signature
    if (node->name) {
        temp = StrPrint("void %s(", node->name);
        String new_code = StrPrint("%s%s", code, temp);
        Free(code);
        Free(temp);
        code = new_code;
    }
    
    // Parameters (simplified)
    temp = StrPrint(") {\n");
    String new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    // Function body
    if (node->children) {
        temp = generate_code(ctx, node->children);
        if (temp) {
            String new_code = StrPrint("%s%s", code, temp);
            Free(code);
            Free(temp);
            code = new_code;
        }
    }
    
    temp = StrPrint("}\n");
    new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    return code;
}

String generate_class(CompilerContext* ctx, ASTNode* node) {
    String code = StrNew("");
    String temp;
    
    if (node->name) {
        temp = StrPrint("typedef struct %s {\n", node->name);
        String new_code = StrPrint("%s%s", code, temp);
        Free(code);
        Free(temp);
        code = new_code;
    }
    
    // Class members (simplified)
    temp = StrPrint("    // Members\n");
    String new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    temp = StrPrint("} %s;\n\n", node->name ? node->name : "Class");
    new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    return code;
}

String generate_variable(CompilerContext* ctx, ASTNode* node) {
    // Simplified variable generation
    return StrNew("");
}

String generate_assignment(CompilerContext* ctx, ASTNode* node) {
    String code = StrNew("");
    String temp;
    
    if (node->left) {
        temp = generate_code(ctx, node->left);
        if (temp) {
            String new_code = StrPrint("%s%s", code, temp);
            Free(code);
            Free(temp);
            code = new_code;
        }
    }
    
    // Assignment operator
    switch (node->value.binary_op) {
        case OP_ASSIGN: temp = StrNew(" = "); break;
        case OP_ADD_ASSIGN: temp = StrNew(" += "); break;
        case OP_SUB_ASSIGN: temp = StrNew(" -= "); break;
        case OP_MUL_ASSIGN: temp = StrNew(" *= "); break;
        case OP_DIV_ASSIGN: temp = StrNew(" /= "); break;
        case OP_MOD_ASSIGN: temp = StrNew(" %= "); break;
        case OP_AND_ASSIGN: temp = StrNew(" &= "); break;
        case OP_OR_ASSIGN: temp = StrNew(" |= "); break;
        case OP_XOR_ASSIGN: temp = StrNew(" ^= "); break;
        case OP_SHL_ASSIGN: temp = StrNew(" <<= "); break;
        case OP_SHR_ASSIGN: temp = StrNew(" >>= "); break;
        default: temp = StrNew(" = "); break;
    }
    
    String new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    if (node->right) {
        temp = generate_code(ctx, node->right);
        if (temp) {
            new_code = StrPrint("%s%s", code, temp);
            Free(code);
            Free(temp);
            code = new_code;
        }
    }
    
    return code;
}

String generate_binary_op(CompilerContext* ctx, ASTNode* node) {
    String code = StrNew("");
    String temp;
    
    if (node->left) {
        temp = generate_code(ctx, node->left);
        if (temp) {
            String new_code = StrPrint("%s%s", code, temp);
            Free(code);
            Free(temp);
            code = new_code;
        }
    }
    
    // Binary operator
    switch (node->value.binary_op) {
        case OP_ADD: temp = StrNew(" + "); break;
        case OP_SUB: temp = StrNew(" - "); break;
        case OP_MUL: temp = StrNew(" * "); break;
        case OP_DIV: temp = StrNew(" / "); break;
        case OP_MOD: temp = StrNew(" % "); break;
        case OP_SHL: temp = StrNew(" << "); break;
        case OP_SHR: temp = StrNew(" >> "); break;
        case OP_AND: temp = StrNew(" & "); break;
        case OP_OR: temp = StrNew(" | "); break;
        case OP_XOR: temp = StrNew(" ^ "); break;
        case OP_EQU: temp = StrNew(" == "); break;
        case OP_NE: temp = StrNew(" != "); break;
        case OP_LT: temp = StrNew(" < "); break;
        case OP_LE: temp = StrNew(" <= "); break;
        case OP_GT: temp = StrNew(" > "); break;
        case OP_GE: temp = StrNew(" >= "); break;
        case OP_AND_AND: temp = StrNew(" && "); break;
        case OP_OR_OR: temp = StrNew(" || "); break;
        case OP_XOR_XOR: temp = StrNew(" ^^ "); break;
        case OP_POWER: temp = StrNew(" ** "); break;
        default: temp = StrNew(" ? "); break;
    }
    
    String new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    if (node->right) {
        temp = generate_code(ctx, node->right);
        if (temp) {
            new_code = StrPrint("%s%s", code, temp);
            Free(code);
            Free(temp);
            code = new_code;
        }
    }
    
    return code;
}

String generate_unary_op(CompilerContext* ctx, ASTNode* node) {
    String code = StrNew("");
    String temp;
    
    // Unary operator
    switch (node->value.unary_op) {
        case OP_PRE_INC: temp = StrNew("++"); break;
        case OP_PRE_DEC: temp = StrNew("--"); break;
        case OP_PLUS: temp = StrNew("+"); break;
        case OP_MINUS: temp = StrNew("-"); break;
        case OP_NOT: temp = StrNew("!"); break;
        case OP_COMP: temp = StrNew("~"); break;
        case OP_DEREF: temp = StrNew("*"); break;
        case OP_ADDR: temp = StrNew("&"); break;
        case OP_SIZEOF: temp = StrNew("sizeof("); break;
        default: temp = StrNew(""); break;
    }
    
    String new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    if (node->left) {
        temp = generate_code(ctx, node->left);
        if (temp) {
            new_code = StrPrint("%s%s", code, temp);
            Free(code);
            Free(temp);
            code = new_code;
        }
    }
    
    if (node->value.unary_op == OP_SIZEOF) {
        temp = StrNew(")");
        new_code = StrPrint("%s%s", code, temp);
        Free(code);
        Free(temp);
        code = new_code;
    }
    
    return code;
}

String generate_call(CompilerContext* ctx, ASTNode* node) {
    String code = StrNew("");
    String temp;
    
    // Function name (from node->name or node->left)
    if (node->name) {
        temp = StrNew(node->name);
        String new_code = StrPrint("%s%s", code, temp);
        Free(code);
        Free(temp);
        code = new_code;
    } else if (node->left) {
        temp = generate_code(ctx, node->left);
        if (temp) {
            String new_code = StrPrint("%s%s", code, temp);
            Free(code);
            Free(temp);
            code = new_code;
        }
    }
    
    temp = StrNew("(");
    String new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    // Arguments (from node->children or node->right)
    ASTNode* arg = node->children ? node->children : node->right;
    while (arg) {
        temp = generate_code(ctx, arg);
        if (temp) {
            new_code = StrPrint("%s%s", code, temp);
            Free(code);
            Free(temp);
            code = new_code;
        }
        
        if (arg->next) {
            temp = StrNew(", ");
            new_code = StrPrint("%s%s", code, temp);
            Free(code);
            Free(temp);
            code = new_code;
        }
        
        arg = arg->next;
    }
    
    temp = StrNew(")");
    new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    return code;
}

String generate_if(CompilerContext* ctx, ASTNode* node) {
    String code = StrNew("");
    String temp;
    
    temp = StrNew("if (");
    String new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    if (node->left) {
        temp = generate_code(ctx, node->left);
        if (temp) {
            new_code = StrPrint("%s%s", code, temp);
            Free(code);
            Free(temp);
            code = new_code;
        }
    }
    
    temp = StrNew(") {\n");
    new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    if (node->right) {
        temp = generate_code(ctx, node->right);
        if (temp) {
            new_code = StrPrint("%s%s", code, temp);
            Free(code);
            Free(temp);
            code = new_code;
        }
    }
    
    temp = StrNew("}\n");
    new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    return code;
}

String generate_while(CompilerContext* ctx, ASTNode* node) {
    String code = StrNew("");
    String temp;
    
    temp = StrNew("while (");
    String new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    if (node->left) {
        temp = generate_code(ctx, node->left);
        if (temp) {
            new_code = StrPrint("%s%s", code, temp);
            Free(code);
            Free(temp);
            code = new_code;
        }
    }
    
    temp = StrNew(") {\n");
    new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    if (node->right) {
        temp = generate_code(ctx, node->right);
        if (temp) {
            new_code = StrPrint("%s%s", code, temp);
            Free(code);
            Free(temp);
            code = new_code;
        }
    }
    
    temp = StrNew("}\n");
    new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    return code;
}

String generate_for(CompilerContext* ctx, ASTNode* node) {
    String code = StrNew("");
    String temp;
    
    temp = StrNew("for (");
    String new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    // Initialization
    if (node->left) {
        temp = generate_code(ctx, node->left);
        if (temp) {
            new_code = StrPrint("%s%s", code, temp);
            Free(code);
            Free(temp);
            code = new_code;
        }
    }
    
    temp = StrNew("; ");
    new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    // Condition
    if (node->right) {
        temp = generate_code(ctx, node->right);
        if (temp) {
            new_code = StrPrint("%s%s", code, temp);
            Free(code);
            Free(temp);
            code = new_code;
        }
    }
    
    temp = StrNew("; ");
    new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    // Increment
    if (node->children) {
        temp = generate_code(ctx, node->children);
        if (temp) {
            new_code = StrPrint("%s%s", code, temp);
            Free(code);
            Free(temp);
            code = new_code;
        }
    }
    
    temp = StrNew(") {\n");
    new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    // Body
    if (node->children && node->children->right) {
        temp = generate_code(ctx, node->children->right);
        if (temp) {
            new_code = StrPrint("%s%s", code, temp);
            Free(code);
            Free(temp);
            code = new_code;
        }
    }
    
    temp = StrNew("}\n");
    new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    return code;
}

String generate_return(CompilerContext* ctx, ASTNode* node) {
    String code = StrNew("");
    String temp;
    
    temp = StrNew("return");
    String new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    if (node->left) {
        temp = StrNew(" ");
        new_code = StrPrint("%s%s", code, temp);
        Free(code);
        Free(temp);
        code = new_code;
        
        temp = generate_code(ctx, node->left);
        if (temp) {
            new_code = StrPrint("%s%s", code, temp);
            Free(code);
            Free(temp);
            code = new_code;
        }
    }
    
    temp = StrNew(";\n");
    new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    return code;
}

String generate_break(CompilerContext* ctx, ASTNode* node) {
    (void)ctx;
    (void)node;
    return StrNew("break;\n");
}

String generate_continue(CompilerContext* ctx, ASTNode* node) {
    (void)ctx;
    (void)node;
    return StrNew("continue;\n");
}

String generate_goto(CompilerContext* ctx, ASTNode* node) {
    String code = StrNew("");
    String temp;
    
    temp = StrNew("goto ");
    String new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    if (node->name) {
        temp = StrPrint("%s;\n", node->name);
        new_code = StrPrint("%s%s", code, temp);
        Free(code);
        Free(temp);
        code = new_code;
    }
    
    return code;
}

String generate_label(CompilerContext* ctx, ASTNode* node) {
    String code = StrNew("");
    String temp;
    
    if (node->name) {
        temp = StrPrint("%s:\n", node->name);
        String new_code = StrPrint("%s%s", code, temp);
        Free(code);
        Free(temp);
        code = new_code;
    }
    
    return code;
}

String generate_asm(CompilerContext* ctx, ASTNode* node) {
    String code = StrNew("");
    String temp;
    
    temp = StrNew("__asm__(\n");
    String new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    if (node->name) {
        temp = StrPrint("\"%s\"\n", node->name);
        new_code = StrPrint("%s%s", code, temp);
        Free(code);
        Free(temp);
        code = new_code;
    }
    
    temp = StrNew(");\n");
    new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    return code;
}

String generate_string(CompilerContext* ctx, ASTNode* node) {
    (void)ctx;
    if (node->name) {
        return StrPrint("\"%s\"", node->name);
    }
    return StrNew("\"\"");
}

String generate_number(CompilerContext* ctx, ASTNode* node) {
    (void)ctx;
    if (node->value.int_val != 0) {
        return StrPrint("%lld", node->value.int_val);
    } else {
        return StrPrint("%.6f", node->value.float_val);
    }
}

String generate_identifier(CompilerContext* ctx, ASTNode* node) {
    (void)ctx;
    if (node->name) {
        return StrNew(node->name);
    }
    return StrNew("");
}

String generate_char_const(CompilerContext* ctx, ASTNode* node) {
    (void)ctx;
    return StrPrint("'%c'", (char)node->value.int_val);
}

String generate_array_access(CompilerContext* ctx, ASTNode* node) {
    String code = StrNew("");
    String temp;
    
    if (node->left) {
        temp = generate_code(ctx, node->left);
        if (temp) {
            String new_code = StrPrint("%s%s", code, temp);
            Free(code);
            Free(temp);
            code = new_code;
        }
    }
    
    temp = StrNew("[");
    String new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    if (node->right) {
        temp = generate_code(ctx, node->right);
        if (temp) {
            new_code = StrPrint("%s%s", code, temp);
            Free(code);
            Free(temp);
            code = new_code;
        }
    }
    
    temp = StrNew("]");
    new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    return code;
}

String generate_member_access(CompilerContext* ctx, ASTNode* node) {
    String code = StrNew("");
    String temp;
    
    if (node->left) {
        temp = generate_code(ctx, node->left);
        if (temp) {
            String new_code = StrPrint("%s%s", code, temp);
            Free(code);
            Free(temp);
            code = new_code;
        }
    }
    
    temp = StrNew(".");
    String new_code = StrPrint("%s%s", code, temp);
    Free(code);
    Free(temp);
    code = new_code;
    
    if (node->right) {
        temp = generate_code(ctx, node->right);
        if (temp) {
            new_code = StrPrint("%s%s", code, temp);
            Free(code);
            Free(temp);
            code = new_code;
        }
    }
    
    return code;
}

String generate_cast(CompilerContext* ctx, ASTNode* node) {
    (void)ctx;
    (void)node;
    return StrNew("(type)");
}

String generate_ternary(CompilerContext* ctx, ASTNode* node) {
    (void)ctx;
    (void)node;
    return StrNew("? :");
}

String generate_range_cmp(CompilerContext* ctx, ASTNode* node) {
    (void)ctx;
    (void)node;
    return StrNew("range");
}

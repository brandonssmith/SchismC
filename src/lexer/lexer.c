#include "holyc.h"
#include <ctype.h>

// Keywords
typedef struct {
    const char* name;
    TokenType token;
} Keyword;

static Keyword keywords[] = {
    {"if", TK_IF},
    {"else", TK_ELSE},
    {"while", TK_WHILE},
    {"for", TK_FOR},
    {"do", TK_DO},
    {"switch", TK_SWITCH},
    {"case", TK_CASE},
    {"default", TK_DEFAULT},
    {"break", TK_BREAK},
    {"continue", TK_CONTINUE},
    {"return", TK_RETURN},
    {"goto", TK_GOTO},
    {"ifdef", TK_IFDEF},
    {"ifndef", TK_IFNDEF},
    {"endif", TK_ENDIF},
    {"define", TK_DEFINE},
    {"include", TK_INCLUDE},
    {"asm", TK_ASM},
    {"class", TK_CLASS},
    {"union", TK_UNION},
    {"public", TK_PUBLIC},
    {"extern", TK_EXTERN},
    {"import", TK_IMPORT},
    {"try", TK_TRY},
    {"catch", TK_CATCH},
    {"throw", TK_THROW},
    {"no_warn", TK_NO_WARN},
    {"reg", TK_REG},
    {"noreg", TK_NOREG},
    {"interrupt", TK_INTERRUPT},
    {"haserrcode", TK_HASERRCODE},
    {"argpop", TK_ARGPOP},
    {"noargpop", TK_NOARGPOP},
    {"lastclass", TK_LASTCLASS},
    // Built-in types
    {"I0", TK_IDENT},
    {"I8", TK_IDENT},
    {"I16", TK_IDENT},
    {"I32", TK_IDENT},
    {"I64", TK_IDENT},
    {"U0", TK_IDENT},
    {"U8", TK_IDENT},
    {"U16", TK_IDENT},
    {"U32", TK_IDENT},
    {"U64", TK_IDENT},
    {"F64", TK_IDENT},
    {"Bool", TK_IDENT},
    {"String", TK_IDENT},
    {"TRUE", TK_IDENT},
    {"FALSE", TK_IDENT},
    {"NULL", TK_IDENT},
    {NULL, TK_EOF}
};

void lex_init(CompilerContext* ctx, FILE* file, const char* filename) {
    ctx->input_file = file;
    ctx->filename = StrNew(filename);
    ctx->line_num = 1;
    ctx->col_num = 1;
    ctx->current_token = TK_EOF;
    ctx->current_string = NULL;
    ctx->current_int = 0;
    ctx->current_float = 0.0;
    ctx->current_char = 0;
    ctx->symbol_table = NULL;
    ctx->ast_root = NULL;
    ctx->output_code = NULL;
    ctx->error_count = 0;
    ctx->warning_count = 0;
    
    // Get the first token
    lex_next_token(ctx);
}

void lex_cleanup(CompilerContext* ctx) {
    if (ctx->filename) {
        Free(ctx->filename);
        ctx->filename = NULL;
    }
    if (ctx->current_string) {
        Free(ctx->current_string);
        ctx->current_string = NULL;
    }
}

static int get_char(CompilerContext* ctx) {
    int ch = fgetc(ctx->input_file);
    if (ch == '\n') {
        ctx->line_num++;
        ctx->col_num = 1;
    } else {
        ctx->col_num++;
    }
    return ch;
}

static void unget_char(CompilerContext* ctx, int ch) {
    ungetc(ch, ctx->input_file);
    if (ch == '\n') {
        ctx->line_num--;
        ctx->col_num = 1; // Will be set correctly by next get_char
    } else {
        ctx->col_num--;
    }
}

static void skip_whitespace(CompilerContext* ctx) {
    int ch;
    while ((ch = get_char(ctx)) != EOF) {
        if (ch == ' ' || ch == '\t' || ch == '\r') {
            continue;
        } else if (ch == '\n') {
            continue;
        } else {
            unget_char(ctx, ch);
            break;
        }
    }
}

static void skip_comment(CompilerContext* ctx) {
    int ch = get_char(ctx);
    if (ch == '/') {
        // Single line comment
        while ((ch = get_char(ctx)) != EOF && ch != '\n') {
            // Skip until end of line
        }
    } else if (ch == '*') {
        // Multi-line comment
        while ((ch = get_char(ctx)) != EOF) {
            if (ch == '*') {
                ch = get_char(ctx);
                if (ch == '/') {
                    break;
                }
                unget_char(ctx, ch);
            }
        }
    } else {
        unget_char(ctx, ch);
    }
}

static TokenType parse_number(CompilerContext* ctx, int first_char) {
    I64 value = first_char - '0';
    int ch;
    bool is_float = false;
    bool is_hex = false;
    bool is_binary = false;
    
    // Check for hex
    if (first_char == '0') {
        ch = get_char(ctx);
        if (ch == 'x' || ch == 'X') {
            is_hex = true;
            value = 0;
            while ((ch = get_char(ctx)) != EOF) {
                if (ch >= '0' && ch <= '9') {
                    value = value * 16 + (ch - '0');
                } else if (ch >= 'a' && ch <= 'f') {
                    value = value * 16 + (ch - 'a' + 10);
                } else if (ch >= 'A' && ch <= 'F') {
                    value = value * 16 + (ch - 'A' + 10);
                } else {
                    unget_char(ctx, ch);
                    break;
                }
            }
            ctx->current_int = value;
            return TK_I64;
        } else if (ch == 'b' || ch == 'B') {
            is_binary = true;
            value = 0;
            while ((ch = get_char(ctx)) != EOF) {
                if (ch == '0') {
                    value = value * 2;
                } else if (ch == '1') {
                    value = value * 2 + 1;
                } else {
                    unget_char(ctx, ch);
                    break;
                }
            }
            ctx->current_int = value;
            return TK_I64;
        } else {
            unget_char(ctx, ch);
        }
    }
    
    // Parse decimal number
    while ((ch = get_char(ctx)) != EOF) {
        if (ch >= '0' && ch <= '9') {
            value = value * 10 + (ch - '0');
        } else if (ch == '.') {
            is_float = true;
            break;
        } else if (ch == 'e' || ch == 'E') {
            is_float = true;
            break;
        } else {
            unget_char(ctx, ch);
            break;
        }
    }
    
    if (is_float) {
        F64 float_value = (F64)value;
        I64 decimal_places = 0;
        
        // Parse decimal part
        while ((ch = get_char(ctx)) != EOF) {
            if (ch >= '0' && ch <= '9') {
                float_value = float_value * 10 + (ch - '0');
                decimal_places++;
            } else if (ch == 'e' || ch == 'E') {
                break;
            } else {
                unget_char(ctx, ch);
                break;
            }
        }
        
        // Parse exponent
        if (ch == 'e' || ch == 'E') {
            bool negative_exp = false;
            I64 exp = 0;
            
            ch = get_char(ctx);
            if (ch == '-') {
                negative_exp = true;
            } else if (ch == '+') {
                // positive exponent
            } else {
                unget_char(ctx, ch);
            }
            
            while ((ch = get_char(ctx)) != EOF) {
                if (ch >= '0' && ch <= '9') {
                    exp = exp * 10 + (ch - '0');
                } else {
                    unget_char(ctx, ch);
                    break;
                }
            }
            
            // Apply exponent
            for (I64 i = 0; i < exp; i++) {
                if (negative_exp) {
                    float_value /= 10.0;
                } else {
                    float_value *= 10.0;
                }
            }
        }
        
        // Apply decimal places
        for (I64 i = 0; i < decimal_places; i++) {
            float_value /= 10.0;
        }
        
        ctx->current_float = float_value;
        return TK_F64;
    } else {
        ctx->current_int = value;
        return TK_I64;
    }
}

static TokenType parse_string(CompilerContext* ctx) {
    String str = NULL;
    I64 len = 0;
    I64 capacity = 16;
    int ch;
    
    str = MAlloc(capacity);
    
    while ((ch = get_char(ctx)) != EOF) {
        if (ch == '"') {
            break;
        } else if (ch == '\\') {
            ch = get_char(ctx);
            switch (ch) {
                case 'n': ch = '\n'; break;
                case 't': ch = '\t'; break;
                case 'r': ch = '\r'; break;
                case '\\': ch = '\\'; break;
                case '"': ch = '"'; break;
                case '0': ch = '\0'; break;
                case 'x':
                case 'X': {
                    // Hex escape
                    I64 hex_val = 0;
                    int hex_ch;
                    for (int i = 0; i < 2; i++) {
                        hex_ch = get_char(ctx);
                        if (hex_ch >= '0' && hex_ch <= '9') {
                            hex_val = hex_val * 16 + (hex_ch - '0');
                        } else if (hex_ch >= 'a' && hex_ch <= 'f') {
                            hex_val = hex_val * 16 + (hex_ch - 'a' + 10);
                        } else if (hex_ch >= 'A' && hex_ch <= 'F') {
                            hex_val = hex_val * 16 + (hex_ch - 'A' + 10);
                        } else {
                            unget_char(ctx, hex_ch);
                            break;
                        }
                    }
                    ch = (char)hex_val;
                    break;
                }
                default:
                    // Keep the character as-is
                    break;
            }
        }
        
        if (len >= capacity - 1) {
            capacity *= 2;
            str = realloc(str, capacity);
        }
        str[len++] = ch;
    }
    
    str[len] = '\0';
    ctx->current_string = str;
    return TK_STR;
}

static TokenType parse_char_const(CompilerContext* ctx) {
    int ch = get_char(ctx);
    I64 value = 0;
    int count = 0;
    
    while (ch != EOF && ch != '\'' && count < 8) {
        if (ch == '\\') {
            ch = get_char(ctx);
            switch (ch) {
                case 'n': ch = '\n'; break;
                case 't': ch = '\t'; break;
                case 'r': ch = '\r'; break;
                case '\\': ch = '\\'; break;
                case '\'': ch = '\''; break;
                case '0': ch = '\0'; break;
                case 'x':
                case 'X': {
                    // Hex escape
                    I64 hex_val = 0;
                    int hex_ch;
                    for (int i = 0; i < 2; i++) {
                        hex_ch = get_char(ctx);
                        if (hex_ch >= '0' && hex_ch <= '9') {
                            hex_val = hex_val * 16 + (hex_ch - '0');
                        } else if (hex_ch >= 'a' && hex_ch <= 'f') {
                            hex_val = hex_val * 16 + (hex_ch - 'a' + 10);
                        } else if (hex_ch >= 'A' && hex_ch <= 'F') {
                            hex_val = hex_val * 16 + (hex_ch - 'A' + 10);
                        } else {
                            unget_char(ctx, hex_ch);
                            break;
                        }
                    }
                    ch = (char)hex_val;
                    break;
                }
                default:
                    // Keep the character as-is
                    break;
            }
        }
        
        value |= ((I64)ch) << (count * 8);
        count++;
        ch = get_char(ctx);
    }
    
    ctx->current_int = value;
    return TK_CHAR_CONST;
}

static TokenType parse_identifier(CompilerContext* ctx, int first_char) {
    String str = NULL;
    I64 len = 0;
    I64 capacity = 16;
    int ch = first_char;
    
    str = MAlloc(capacity);
    
    while (ch != EOF && (isalnum(ch) || ch == '_')) {
        if (len >= capacity - 1) {
            capacity *= 2;
            str = realloc(str, capacity);
        }
        str[len++] = ch;
        ch = get_char(ctx);
    }
    
    if (ch != EOF) {
        unget_char(ctx, ch);
    }
    
    str[len] = '\0';
    ctx->current_string = str;
    
    // Check if it's a keyword
    for (int i = 0; keywords[i].name != NULL; i++) {
        if (strcmp(str, keywords[i].name) == 0) {
            Free(str);
            ctx->current_string = NULL;
            return keywords[i].token;
        }
    }
    
    return TK_IDENT;
}

TokenType lex_next_token(CompilerContext* ctx) {
    int ch;
    
    while (1) {
        skip_whitespace(ctx);
        ch = get_char(ctx);
        
        if (ch == EOF) {
            return ctx->current_token = TK_EOF;
        }
        
        if (ch == '/') {
            int next_ch = get_char(ctx);
            if (next_ch == '/' || next_ch == '*') {
                unget_char(ctx, next_ch);
                skip_comment(ctx);
                continue;
            } else {
                unget_char(ctx, next_ch);
                return ctx->current_token = '/';
            }
        }
        
        if (ch == '"') {
            return ctx->current_token = parse_string(ctx);
        }
        
        if (ch == '\'') {
            return ctx->current_token = parse_char_const(ctx);
        }
        
        if (ch >= '0' && ch <= '9') {
            return ctx->current_token = parse_number(ctx, ch);
        }
        
        if (isalpha(ch) || ch == '_') {
            return ctx->current_token = parse_identifier(ctx, ch);
        }
        
        // Handle operators
        switch (ch) {
            case '+':
                ch = get_char(ctx);
                if (ch == '+') return ctx->current_token = TK_PLUS_PLUS;
                if (ch == '=') return ctx->current_token = TK_ADD_EQU;
                unget_char(ctx, ch);
                return ctx->current_token = '+';
                
            case '-':
                ch = get_char(ctx);
                if (ch == '-') return ctx->current_token = TK_MINUS_MINUS;
                if (ch == '=') return ctx->current_token = TK_SUB_EQU;
                if (ch == '>') return ctx->current_token = TK_DEREFERENCE;
                unget_char(ctx, ch);
                return ctx->current_token = '-';
                
            case '*':
                ch = get_char(ctx);
                if (ch == '=') return ctx->current_token = TK_MUL_EQU;
                unget_char(ctx, ch);
                return ctx->current_token = '*';
                
            case '/':
                ch = get_char(ctx);
                if (ch == '=') return ctx->current_token = TK_DIV_EQU;
                unget_char(ctx, ch);
                return ctx->current_token = '/';
                
            case '%':
                ch = get_char(ctx);
                if (ch == '=') return ctx->current_token = TK_MOD_EQU;
                unget_char(ctx, ch);
                return ctx->current_token = '%';
                
            case '&':
                ch = get_char(ctx);
                if (ch == '&') return ctx->current_token = TK_AND_AND;
                if (ch == '=') return ctx->current_token = TK_AND_EQU;
                unget_char(ctx, ch);
                return ctx->current_token = '&';
                
            case '|':
                ch = get_char(ctx);
                if (ch == '|') return ctx->current_token = TK_OR_OR;
                if (ch == '=') return ctx->current_token = TK_OR_EQU;
                unget_char(ctx, ch);
                return ctx->current_token = '|';
                
            case '^':
                ch = get_char(ctx);
                if (ch == '^') return ctx->current_token = TK_XOR_XOR;
                if (ch == '=') return ctx->current_token = TK_XOR_EQU;
                unget_char(ctx, ch);
                return ctx->current_token = '^';
                
            case '=':
                ch = get_char(ctx);
                if (ch == '=') return ctx->current_token = TK_EQU_EQU;
                unget_char(ctx, ch);
                return ctx->current_token = '=';
                
            case '!':
                ch = get_char(ctx);
                if (ch == '=') return ctx->current_token = TK_NOT_EQU;
                unget_char(ctx, ch);
                return ctx->current_token = '!';
                
            case '<':
                ch = get_char(ctx);
                if (ch == '<') {
                    ch = get_char(ctx);
                    if (ch == '=') return ctx->current_token = TK_SHL_EQU;
                    unget_char(ctx, ch);
                    return ctx->current_token = TK_SHL;
                }
                if (ch == '=') return ctx->current_token = TK_LESS_EQU;
                unget_char(ctx, ch);
                return ctx->current_token = '<';
                
            case '>':
                ch = get_char(ctx);
                if (ch == '>') {
                    ch = get_char(ctx);
                    if (ch == '=') return ctx->current_token = TK_SHR_EQU;
                    unget_char(ctx, ch);
                    return ctx->current_token = TK_SHR;
                }
                if (ch == '=') return ctx->current_token = TK_GREATER_EQU;
                unget_char(ctx, ch);
                return ctx->current_token = '>';
                
            case '.':
                ch = get_char(ctx);
                if (ch == '.') {
                    ch = get_char(ctx);
                    if (ch == '.') return ctx->current_token = TK_ELLIPSIS;
                    unget_char(ctx, ch);
                    return ctx->current_token = TK_DOT_DOT;
                }
                unget_char(ctx, ch);
                return ctx->current_token = '.';
                
            case ':':
                ch = get_char(ctx);
                if (ch == ':') return ctx->current_token = TK_DBL_COLON;
                unget_char(ctx, ch);
                return ctx->current_token = ':';
                
            case '#':
                // Preprocessor directive - skip for now
                while ((ch = get_char(ctx)) != EOF && ch != '\n') {
                    // Skip to end of line
                }
                continue;
                
            default:
                return ctx->current_token = ch;
        }
    }
}


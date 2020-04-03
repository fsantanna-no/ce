#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"

Lexer CUR = { NULL,-1,0,0,{} };
Lexer OLD = { NULL,-1,0,0,{} };

static char* reserved[] = {
    "func", "let"
};

int is_reserved (TK_val* val) {
    int N = sizeof(reserved) / sizeof(reserved[0]);
    for (int i=0; i<N; i++) {
        if (!strcmp(val->s, reserved[i])) {
            return TK_RESERVED+1 + i;
        }
    }
    return 0;
}

int lexer_tk2len (Tk* tk) {
    switch (tk->sym) {
        case TK_VAR:
            return strlen(tk->val.s);
        default:
            return 1;
    }
}

const char* lexer_tk2str (Tk* tk) {
    static char str[512];
    switch (tk->sym) {
        case TK_EOF:
            sprintf(str, "end of file");
            break;
        case TK_LINE:
            sprintf(str, "new line");
            break;
        case TK_VAR:
            sprintf(str, "`%s`", tk->val.s);
            break;
        default:
            sprintf(str, "`%c`", tk->sym);
            break;
    }
    return str;
}

TK lexer_ (TK_val* val) {
    while (1) {
        int c = fgetc(CUR.buf);
//printf("0> %c\n", c);
        switch (c)
        {
            case ' ':
                break;

            case '(':
            case ')':
                return c;

            case EOF:
                return TK_EOF;

            case '\n':
                return TK_LINE;

            case ':':
                c = fgetc(CUR.buf);
                if (c == ':') {
                    return TK_DECL;
                } else {
                    ungetc(c, CUR.buf);
                    return ':';
                }

            case '-':
                c = fgetc(CUR.buf);
                if (c == '-') {
                    while (1) {
                        c = fgetc(CUR.buf);
                        if (c == EOF) {
                            break;      // EOF stops comment
                        }
                        if (c=='\n' || c=='\r') {
                            ungetc(c, CUR.buf);
                            break;      // NEWLINE stops comment
                        }
                    }
                    return TK_COMMENT;
                }
                return TK_NONE;

            case '=':
                return c;

            default:

                if (!isalpha(c)) {
                    return TK_NONE;
                }

                int i = 0;
                while (isalnum(c) || c=='_' || c=='\'' || c=='?' || c=='!') {
                    val->s[i++] = c;
                    c = fgetc(CUR.buf);
                }
                val->s[i] = '\0';
                ungetc(c, CUR.buf);

                int key = is_reserved(val);
                if (key) {
                    return key;
                }

                return (islower(val->s[0]) ? TK_VAR : TK_DATA);
        }
    }
}

Tk lexer () {
    Tk ret;
    TK tk = lexer_(&ret.val);
    ret.sym = tk;
    return ret;
}

#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"

Lexer NXT = { NULL,0,-1,0,0,{} };
Lexer PRV = { NULL,0,-1,0,0,{} };

static char* reserved[] = {
    "func", "let", "set"
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
        int c = fgetc(NXT.buf);
//printf("0> [%c] [%d]\n", c, c);
        switch (c)
        {
            case '(':
            case ')':
                return c;

            case EOF:
                return TK_EOF;

            case '\n': {
                int i = 0;
                while (1) {
                    c = fgetc(NXT.buf);
                    if (c != ' ') {
                        ungetc(c, NXT.buf);
                        break;
                    }
                    i++;
                }
                if (i%4 != 0) {
                    return TK_ERR;
                }
                val->n = i/4;
                return TK_LINE;
            }

            case ':':
                c = fgetc(NXT.buf);
                if (c == ':') {
                    return TK_DECL;
                } else {
                    ungetc(c, NXT.buf);
                    return ':';
                }

            case '-':
                c = fgetc(NXT.buf);
                if (c == '-') {
                    while (1) {
                        c = fgetc(NXT.buf);
                        if (c == EOF) {
                            break;      // EOF stops comment
                        }
                        if (c=='\n' || c=='\r') {
                            ungetc(c, NXT.buf);
                            break;      // NEWLINE stops comment
                        }
                    }
                    return TK_COMMENT;
                }
                return TK_ERR;

            case '=':
                return c;

            default:

                if (!isalpha(c)) {
                    return TK_ERR;
                }

                int i = 0;
                while (isalnum(c) || c=='_' || c=='\'' || c=='?' || c=='!') {
                    val->s[i++] = c;
                    c = fgetc(NXT.buf);
                }
                val->s[i] = '\0';
                ungetc(c, NXT.buf);

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
    while (1) {
        int c = fgetc(NXT.buf);
        if (c != ' ') {
            ungetc(c, NXT.buf);
            break;
        }
    }
    ret.sym = tk;
    return ret;
}

#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"

State NXT = { NULL,NULL,0,-1,0,0,{} };
State PRV = { NULL,NULL,0,-1,0,0,{} };

static char* reserved[] = {
    "data", "func", "let", "set"
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
        case TK_IDVAR:
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
        int c = fgetc(NXT.inp);
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
                    c = fgetc(NXT.inp);
                    if (c != ' ') {
                        ungetc(c, NXT.inp);
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
                c = fgetc(NXT.inp);
                if (c == ':') {
                    return TK_DECL;
                } else {
                    ungetc(c, NXT.inp);
                    return ':';
                }

            case '-':
                c = fgetc(NXT.inp);
                if (c == '-') {
                    while (1) {
                        c = fgetc(NXT.inp);
                        if (c == EOF) {
                            break;      // EOF stops comment
                        }
                        if (c=='\n' || c=='\r') {
                            ungetc(c, NXT.inp);
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
                    c = fgetc(NXT.inp);
                }
                val->s[i] = '\0';
                ungetc(c, NXT.inp);

                int key = is_reserved(val);
                if (key) {
                    return key;
                }

                return (islower(val->s[0]) ? TK_IDVAR : TK_DATA);
        }
    }
}

Tk lexer () {
    Tk ret;
    TK tk = lexer_(&ret.val);
    while (1) {
        int c = fgetc(NXT.inp);
        if (c != ' ') {
            ungetc(c, NXT.inp);
            break;
        }
    }
    ret.sym = tk;
    return ret;
}

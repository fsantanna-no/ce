#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"

State_All ALL = { NULL,NULL,{},0 };
State_Tok NXT = { -1,0,0,{} };
State_Tok PRV = { -1,0,0,{} };

static char* reserved[] = {
    "break", "match", "data", "else",   "func", "if",  "let", "loop",
    "mut",   "new",  "pass", "return", "set",  "val", "where"
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
        case TK_ERR:
            sprintf(str, "`%c`", tk->val.n);
            break;
        case EOF:
            sprintf(str, "end of file");
            break;
        case '\n':
            sprintf(str, "end of line");
            break;
        case TK_RAW:
            sprintf(str, "`{...}`");
            break;
        case TK_DECL:
            sprintf(str, "`::`");
            break;
        case TK_ARROW:
            sprintf(str, "`->`");
            break;
        case TK_ARG:
            sprintf(str, "`...`");
            break;
        case TK_IDVAR:
        case TK_IDDATA:
            sprintf(str, "`%s`", tk->val.s);
            break;
        default:
            if (tk->sym < TK_CHAR) {
                sprintf(str, "`%c`", tk->sym);
            } else if (tk->sym > TK_RESERVED) {
                sprintf(str, "`%s`", reserved[tk->sym-TK_RESERVED-1]);
            } else {
//printf("%d\n", tk->sym);
                assert(0 && "TODO");
            }
            break;
    }
    return str;
}

TK lexer_ (TK_val* val) {
    int c = fgetc(ALL.inp);
//printf("0> [%c] [%d]\n", c, c);
    switch (c)
    {
        case '(':
        case ')':
        case ',':
        case '_':
        case '~':
        case '?':
        case '=':
        case EOF:
        case '\n':
            return c;

        case ' ': {
            int i = 1;
            while (1) {
                c = fgetc(ALL.inp);
                if (c != ' ') {
                    ungetc(c, ALL.inp);
                    break;
                }
                i++;
            }
            val->n = i;
            return ' ';
        }

        case '{': {
            int i = 0;
            int n = 1;
            while (1) {
                c = fgetc(ALL.inp);
                switch (c) {
                    case '{':
                        n++;
                        break;
                    case '}':
                        if (--n == 0) {
                            val->s[i] = '\0';
                            return TK_RAW;
                        }
                        break;
                    case '\n':
                        return TK_ERR;
                }
                val->s[i++] = c;
            }
        }

        case ':':
            c = fgetc(ALL.inp);
            if (c == ':') {
                return TK_DECL;
            } else {
                ungetc(c, ALL.inp);
                return ':';
            }

        case '.': {
            int c2 = fgetc(ALL.inp);
            int c3 = fgetc(ALL.inp);
            if (c2=='.' && c3=='.') {
                return TK_ARG;
            }
            return TK_ERR;
        }

        case '-':
            c = fgetc(ALL.inp);
            if (c == '-') {
                while (1) {
                    c = fgetc(ALL.inp);
                    if (c == EOF) {
                        break;      // EOF stops comment
                    }
                    if (c == '\n') {
                        ungetc(c, ALL.inp);
                        break;      // NEWLINE stops comment
                    }
                }
                return TK_COMMENT;
            } else if (c == '>') {
                return TK_ARROW;
            }
            return TK_ERR;

        default:

            if (!isalpha(c)) {
                val->n = c;
                return TK_ERR;
            }

            int i = 0;
            while (isalnum(c) || c=='_' || c=='\'' || c=='?' || c=='!') {
                val->s[i++] = c;
                c = fgetc(ALL.inp);
            }
            val->s[i] = '\0';
            ungetc(c, ALL.inp);

            int key = is_reserved(val);
            if (key) {
                return key;
            }

            return (islower(val->s[0]) ? TK_IDVAR : TK_IDDATA);
    }
    assert(0 && "bug found");
}

Tk lexer () {
    static TK prv = '\n';
    Tk ret;
    TK tk = lexer_(&ret.val);
    if (tk==EOF && prv!=EOF && prv!='\n') {
        tk = '\n';
    }
    prv = tk;
    if (tk != '\n') {   // ignore spaces not starting lines
        while (1) {
            int c = fgetc(ALL.inp);
            if (c != ' ') {
                ungetc(c, ALL.inp);
                break;
            }
        }
    }
    ret.sym = tk;
//printf(": %d %c\n", ret.sym, ret.sym);
    return ret;
}

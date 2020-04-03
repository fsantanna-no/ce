#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "lexer.h"

static char* reserved[] = {
    "let"
};

int is_reserved (void) {
    int N = sizeof(reserved) / sizeof(reserved[0]);
    for (int i=0; i<N; i++) {
        if (!strcmp(LX.val.s, reserved[i])) {
            return TK_RESERVED+1 + i;
        }
    }
    return 0;
}

TK lexer () {
    while (1) {
        int c = fgetc(LX.buf);
printf("0> %c\n", c);
        switch (c)
        {
            case ' ':
            case '\t':
                break;

            case '(':
            case ')':
                return c;

            case EOF:
                return TK_EOF;

            case '\n':
                return TK_LINE;

            case '\r':
                c = fgetc(LX.buf);
                LX.val.n = 2;
                if (c != '\n') {
                    ungetc(c, LX.buf);
                    LX.val.n--;
                }
                return TK_LINE;

            case ':':
                c = fgetc(LX.buf);
                return (c == ':') ? TK_DECL : TK_NONE;

            case '-':
                c = fgetc(LX.buf);
                if (c == '-') {
                    while (1) {
                        c = fgetc(LX.buf);
                        if (c == EOF) {
                            break;      // EOF stops comment
                        }
                        if (c=='\n' || c=='\r') {
                            ungetc(c, LX.buf);
                            break;      // NEWLINE stops comment
                        }
                    }
                    return TK_COMMENT;
                }
                return TK_NONE;

            default:

                if (!isalpha(c)) {
                    return TK_NONE;
                }

                int i = 0;
                while (isalnum(c) || c=='_' || c=='\'' || c=='?' || c=='!') {
                    LX.val.s[i++] = c;
                    c = fgetc(LX.buf);
                }
                LX.val.s[i] = '\0';
                ungetc(c, LX.buf);

                int key = is_reserved();
                if (key) {
                    return key;
                }

                return (islower(LX.val.s[0]) ? TK_VAR : TK_DATA);
        }
    }
}

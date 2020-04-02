#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "lex.h"

static char* reserved[] = {
    "let"
};

int is_reserved (void) {
    int N = sizeof(reserved) / sizeof(reserved[0]);
    for (int i=0; i<N; i++) {
        if (!strcmp(lex_value, reserved[i])) {
            return TK_RESERVED+1 + i;
        }
    }
    return 0;
}

TK lex (FILE* buf) {
    while (1) {
        int c = fgetc(buf);
printf("0> %c\n", c);
        switch (c)
        {
            case ' ':
            case '\t':
                break;

            case EOF:
                return TK_EOF;

            case '\n':
                return TK_NEWLINE;

            case '\r':
                c = fgetc(buf);
                if (c != '\n') {
                    ungetc(c, buf);
                }
                return TK_NEWLINE;

            case '-':
                c = fgetc(buf);
                if (c == '-') {
                    while (1) {
                        c = fgetc(buf);
                        if (c == EOF) {
                            break;      // EOF stops comment
                        }
                        if (c=='\n' || c=='\r') {
                            ungetc(c, buf);
                            break;      // NEWLINE stops comment
                        }
                    }
                    return TK_COMMENT;
                }
                return TK_MINUS;

            default:

                if (isalpha(c)) {
                    int i = 0;
                    while (isalnum(c) || c=='_' || c=='\'' || c=='?' || c=='!') {
                        lex_value[i++] = c;
                        c = fgetc(buf);
                    }
                    lex_value[i] = '\0';
                    ungetc(c, buf);

                    int key = is_reserved();
                    if (key) {
                        return key;
                    }

                    return (islower(lex_value[0]) ? TK_VAR : TK_DATA);
                    return TK_VAR;
                }
        }
    }
}

#include <stdio.h>

#include "lex.h"

TK lex (FILE* buf) {
    while (1) {
        int c = fgetc(buf);
        printf("0> %c\n", c);
        switch (c) {
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
        }
    }
}

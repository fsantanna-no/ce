#include <stdio.h>

#include "lexer.h"
#include "parser.h"

EXP parser_exp (FILE* buf) {
    TK tk = lexer(buf);
    switch (tk) {
        case '(':
            tk = lexer(buf);
            if (tk == ')') {
                return EXP_UNIT;
            } else {
                return EXP_NONE;
            }
        case TK_VAR:
            return EXP_VAR;
        case TK_DATA:
            return EXP_CONS;
    }
    return EXP_NONE;
}

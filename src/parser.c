#include <stdio.h>

#include "lexer.h"
#include "parser.h"

EXP parser_exp () {
    TK tk = lexer();
    switch (tk) {
        case '(':
            tk = lexer();
            if (tk == ')') {
                return EXP_UNIT;
            } else {
                ungetc(tk, LX.buf);
                EXP ret = parser_exp();
                tk = lexer(LX.buf);
                if (tk == ')') {
                    return ret;
                } else {
                    return EXP_NONE;
                }
            }
        case TK_VAR:
            return EXP_VAR;
        case TK_DATA:
            return EXP_CONS;
    }
    return EXP_NONE;
}

#include <stdio.h>

#include "lexer.h"
#include "parser.h"

EXP parser_expr (FILE* buf) {
    TK tk = lexer(buf);
    switch (tk) {
        case TK_VAR:
            return EXP_VAR;
        case TK_DATA:
            return EXP_CONS;
    }
    return EXP_NONE;
}

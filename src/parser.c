#include <stdio.h>

#include "lexer.h"
#include "parser.h"

EXP parser_expr (FILE* buf) {
    TK tk = lexer(buf);
    if (tk == TK_VAR) {
        return EXP_VAR;
    }
    return EXP_NONE;
}

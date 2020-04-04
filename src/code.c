#include "lexer.h"
#include "parser.h"
#include "code.h"

void code_expr (int spc, Expr e) {
    for (int i=0; i<spc; i++) printf(" ");
    switch (e.sub) {
        case EXPR_VAR:
            fputs(e.tk.val.s, NXT.out);
            break;
    }
}

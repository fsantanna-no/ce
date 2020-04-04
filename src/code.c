#include <assert.h>
#include "lexer.h"
#include "parser.h"
#include "code.h"

void code_type (Type tp) {
    switch (tp.sub) {
        case TYPE_UNIT:
            fputs("int /* () */", NXT.out);
            break;
        default:
            assert(0 && "TODO");
    }
}

void code_expr (int spc, Expr e) {
    switch (e.sub) {
        case EXPR_VAR:
            fputs(e.tk.val.s, NXT.out);
            break;
    }
}

void code_decls (int spc, Decls ds) {
    for (int i=0; i<ds.size; i++) {
        for (int j=0; j<spc; j++) fputs(" ", NXT.out);
        code_type(ds.vec[i].type);
        fputs(" ", NXT.out);
        fputs(ds.vec[i].var.val.s, NXT.out);
        fputs(";\n", NXT.out);
    }
}

void code_block (int spc, Block blk) {
    code_decls(spc, blk.decls);
    code_expr (spc, blk.expr);
}

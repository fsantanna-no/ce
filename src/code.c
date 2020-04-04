#include <assert.h>
#include "lexer.h"
#include "parser.h"
#include "code.h"

void code_ret (const char* ret) {
    if (ret != NULL) {
        fputs("ret = ", NXT.out);
    }
}

void code_type (Type tp) {
    switch (tp.sub) {
        case TYPE_UNIT:
            fputs("int /* () */", NXT.out);
            break;
        default:
            assert(0 && "TODO");
    }
}

void code_expr (int spc, Expr e, const char* ret) {
    switch (e.sub) {
        case EXPR_VAR:
            code_ret(ret);
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

void code_block (int spc, Block blk, const char* ret) {
    code_decls(spc, blk.decls);
    for (int i=0; i<spc; i++) fputs(" ", NXT.out);
    code_expr (spc, blk.expr, ret);
    fputs(";\n", NXT.out);
}

void code (Block blk) {
    fputs (
        "#include <stdio.h>\n"
        "int main (void) {\n"
        "    int ret;\n",
        NXT.out
    );
    code_block(4, blk, "ret");
    fprintf(NXT.out, "    printf(\"%%d\\n\", ret);\n");
    fputs("}\n", NXT.out);
}

void compile (const char* inp) {
    FILE* f = popen("gcc -xc -", "w");
    assert(f != NULL);
    fputs(inp, f);
    fclose(f);
}

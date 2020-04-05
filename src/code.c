#include <assert.h>
#include "lexer.h"
#include "parser.h"
#include "code.h"

void code_ret (const char* ret) {
    if (ret != NULL) {
        fputs("ret = ", ALL.out);
    }
}

void code_type (Type tp) {
    switch (tp.sub) {
        case TYPE_UNIT:
            fputs("int /* () */", ALL.out);
            break;
        case TYPE_DATA:
            fputs(tp.tk.val.s, ALL.out);
            break;
    }
}

void code_expr (int spc, Expr e, const char* ret) {
    switch (e.sub) {
        case EXPR_VAR:
            code_ret(ret);
            fputs(e.tk.val.s, ALL.out);
            break;
    }
}

void code_decls (int spc, Decls ds) {
    for (int i=0; i<ds.size; i++) {
        for (int j=0; j<spc; j++) fputs(" ", ALL.out);
        code_type(ds.vec[i].type);
        fputs(" ", ALL.out);
        fputs(ds.vec[i].var.val.s, ALL.out);
        fputs(";\n", ALL.out);
    }
}

void code_block (int spc, Block blk, const char* ret) {
    code_decls(spc, blk.decls);
    for (int i=0; i<spc; i++) fputs(" ", ALL.out);
    code_expr (spc, blk.expr, ret);
    fputs(";\n", ALL.out);
}

void code (Block blk) {
    fputs (
        "#include <stdio.h>\n"
        "int main (void) {\n"
        "    int ret;\n",
        ALL.out
    );
    code_block(4, blk, "ret");
    fprintf(ALL.out, "    printf(\"%%d\\n\", ret);\n");
    fputs("}\n", ALL.out);
}

void compile (const char* inp) {
    FILE* f = popen("gcc -xc -", "w");
    assert(f != NULL);
    fputs(inp, f);
    fclose(f);
}

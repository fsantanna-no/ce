#include <assert.h>
#include "lexer.h"
#include "parser.h"
#include "code.h"

void code_spc (int spc) {
    for (int i=0; i<spc; i++) {
        fputs(" ", ALL.out);
    }
}

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
        case EXPR_UNIT:
            code_ret(ret);
            fputs("0", ALL.out);
            break;
        case EXPR_VAR:
            code_ret(ret);
            fputs(e.tk.val.s, ALL.out);
            break;
        case EXPR_SET:
            fputs(e.Set.var.val.s, ALL.out);
            fputs(" = ", ALL.out);
            code_expr(spc, *e.Set.expr, ret);
            break;
        default:
            assert(0 && "TODO");
    }
}

void code_decl (int spc, Decl d) {
    code_type(d.type);
    fputs(" ", ALL.out);
    fputs(d.var.val.s, ALL.out);
}

void code_decls (int spc, Decls ds) {
    for (int i=0; i<ds.size; i++) {
        code_spc(spc);
        code_decl(spc, ds.vec[i]);
        fputs(";\n", ALL.out);
    }
}

void code_block (int spc, Block blk, const char* ret) {
    code_decls(spc, blk.decls);
    code_spc(spc);
    code_expr (spc, blk.expr, ret);
    fputs(";\n", ALL.out);
}

void code_prog (int spc, Prog prog) {
    for (int i=0; i<prog.size; i++) {
        Glob g = prog.vec[i];
        code_spc(spc);
        switch (g.sub) {
            case GLOB_DATAS:
                // TODO
                break;
            case GLOB_DECL:
                code_decl(spc, g.decl);
                break;
            case GLOB_EXPR:
                code_expr(spc, g.expr, NULL);
                break;
        }
        fputs(";\n", ALL.out);
    }
}

void code (Prog prog) {
    fputs (
        "#include <stdio.h>\n"
        "int main (void) {\n"
        "    int ret;\n",
        ALL.out
    );
    code_prog(4, prog);
    fprintf(ALL.out, "    printf(\"%%d\\n\", ret);\n");
    fputs("}\n", ALL.out);
}

void compile (const char* inp) {
    FILE* f = popen("gcc -xc -", "w");
    assert(f != NULL);
    fputs(inp, f);
    fclose(f);
}

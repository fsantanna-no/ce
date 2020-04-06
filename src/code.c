#include <assert.h>
#include <string.h>
#include <ctype.h>

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

///////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////

char* strupper (const char* src) {
    static char dst[256];
    assert(strlen(src) < sizeof(dst));
    for (int i=0; i<strlen(src); i++) {
        dst[i] = toupper(src[i]);
    }
    dst[strlen(src)] = '\0';
    return dst;
}

void code_data (Data data) {
    char* id = data.tk.val.s;
    char ID[256];
    assert(strlen(id) < sizeof(ID));
    strcpy(ID, strupper(id));

    // TODO: asserts || strncat
    char subs[1024] = "";
    for (int i=0; i<data.size; i++) {
        char* v = data.vec[i].tk.val.s;
        strcat(subs, "    ");
        strcat(subs, ID);
        strcat(subs, "_");
        strcat(subs, strupper(v));  // TODO: assert strupper
        if (i < data.size-1) {
            strcat(subs, ",");
        }
        strcat(subs, "\n");
    }


    char conss[1024] = "";
    for (int i=0; i<data.size; i++) {
        Cons v = data.vec[i];
        if (v.type.sub != TYPE_UNIT) {
            assert(0 && "TODO");
        }
    }

    fprintf (ALL.out,
        "typedef enum {\n"
        "%s"
        "} %s;\n"
        "\n"
        "typedef struct %s {\n"
        "    %s sub;\n"
        "    union {\n"
        "%s"
        "    };\n"
        "} %s;\n",
        subs,
        ID,
        id,
        ID,
        conss,
        id
    );
}

///////////////////////////////////////////////////////////////////////////////

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
        case EXPR_CONS: {
            char tmp[256];
            strcpy(tmp, e.tk.val.s);
            code_ret(ret);
            fprintf(ALL.out, "(%s) { %s }", strtok(tmp,"_"), strupper(e.tk.val.s));
            break;
        }
        case EXPR_SET:
            fputs(e.Set.var.val.s, ALL.out);
            fputs(" = ", ALL.out);
            code_expr(spc, *e.Set.val, ret);
            break;
        case EXPR_BLOCK:
            code_decls(spc, *e.Block.decls);
            code_spc(spc);
            code_expr (spc, *e.Block.ret, ret);
            break;
        default:
            //printf("%d\n", e.sub);
            assert(0 && "TODO");
    }
}

///////////////////////////////////////////////////////////////////////////////

void code_decl (int spc, Decl d) {
    code_type(d.type);
    fputs(" ", ALL.out);
    fputs(d.var.val.s, ALL.out);
    if (d.set != NULL) {
        fputs(" = ", ALL.out);
        code_expr(spc, *d.set, NULL);
    }
    fputs(";\n", ALL.out);
}

void code_decls (int spc, Decls ds) {
    for (int i=0; i<ds.size; i++) {
        code_spc(spc);
        code_decl(spc, ds.vec[i]);
    }
}

///////////////////////////////////////////////////////////////////////////////

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
                fputs(";\n", ALL.out);
                break;
        }
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

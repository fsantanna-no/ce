#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "lexer.h"
#include "parser.h"
#include "code.h"

FILE* stropen (const char* mode, size_t size, char* str) {
    size = (size != 0) ? size : strlen(str);
    return fmemopen(str, size, mode);
}

void t_lexer (void) {
    {
        NXT.inp = stropen("r", 0, "-- foobar");
        assert(lexer().sym == TK_COMMENT);
        assert(lexer().sym == TK_EOF);
        fclose(NXT.inp);
    }
    {
        NXT.inp = stropen("r", 0, "-- c1\n--c2\n\n");
        assert(lexer().sym == TK_COMMENT);
        assert(lexer().sym == TK_LINE);
        assert(lexer().sym == TK_COMMENT);
        assert(lexer().sym == TK_LINE);
        assert(lexer().sym == TK_LINE);
        assert(lexer().sym == TK_EOF);
        fclose(NXT.inp);
    }
    {
        NXT.inp = stropen("r", 0, "\n  \n");
        assert(lexer().sym == TK_ERR);
        assert(lexer().sym == TK_LINE);
        fclose(NXT.inp);
    }
    {
        NXT.inp = stropen("r", 0, "c1\nc2 c3'  \n    \nc4");
        Tk tk1 = lexer(); assert(tk1.sym == TK_VAR); assert(!strcmp(tk1.val.s, "c1"));
        assert(lexer().sym == TK_LINE);
        Tk tk3 = lexer(); assert(tk3.sym == TK_VAR); assert(!strcmp(tk3.val.s, "c2"));
        Tk tk4 = lexer(); assert(tk4.sym == TK_VAR); assert(!strcmp(tk4.val.s, "c3'"));
        assert(lexer().sym == TK_LINE);
        assert(lexer().sym == TK_LINE);
        Tk tk5 = lexer(); assert(tk5.sym == TK_VAR); assert(!strcmp(tk5.val.s, "c4"));
        assert(lexer().sym == TK_EOF);
        fclose(NXT.inp);
    }
    {
        NXT.inp = stropen("r", 0, "c1 C1 C'a a'? C!!");
        Tk tk1 = lexer(); assert(tk1.sym == TK_VAR);  assert(!strcmp(tk1.val.s, "c1"));
        Tk tk2 = lexer(); assert(tk2.sym == TK_DATA); assert(!strcmp(tk2.val.s, "C1"));
        Tk tk3 = lexer(); assert(tk3.sym == TK_DATA); assert(!strcmp(tk3.val.s, "C'a"));
        Tk tk4 = lexer(); assert(tk4.sym == TK_VAR);  assert(!strcmp(tk4.val.s, "a'?"));
        Tk tk5 = lexer(); assert(tk5.sym == TK_DATA); assert(!strcmp(tk5.val.s, "C!!"));
        assert(lexer().sym == TK_EOF);
        fclose(NXT.inp);
    }
    {
        NXT.inp = stropen("r", 0, "let xlet letx");
        assert(lexer().sym == TK_LET);
        Tk tk1 = lexer(); assert(tk1.sym == TK_VAR); assert(!strcmp(tk1.val.s, "xlet"));
        Tk tk2 = lexer(); assert(tk2.sym == TK_VAR); assert(!strcmp(tk2.val.s, "letx"));
        assert(lexer().sym == TK_EOF);
        fclose(NXT.inp);
    }
    {
        NXT.inp = stropen("r", 0, ": :: :");
        assert(lexer().sym == ':');
        assert(lexer().sym == TK_DECL);
        assert(lexer().sym == ':');
        fclose(NXT.inp);
    }
}

void t_parser_type (void) {
    {
        init(NULL, stropen("r", 0, "()"));
        assert(parser_type().sub == TYPE_UNIT);
        fclose(NXT.inp);
    }
}

void t_parser_expr (void) {
    // PARENS
    {
        init(NULL, stropen("r", 0, "(())"));
        assert(parser_expr().sub == EXPR_UNIT);
        fclose(NXT.inp);
    }
    {
        init(NULL, stropen("r", 0, "( ( ) )"));
        assert(parser_expr().sub == EXPR_UNIT);
        fclose(NXT.inp);
    }
    {
        init(NULL, stropen("r", 0, "("));
        Expr e = parser_expr();
        assert(e.sub == EXPR_ERR); assert(!strcmp(e.err.msg, "(ln 1, col 2): expected `)` : have end of file"));
        fclose(NXT.inp);
    }
    {
        init(NULL, stropen("r", 0, "(("));
        Expr e = parser_expr();
        assert(e.sub == EXPR_ERR); assert(!strcmp(e.err.msg, "(ln 1, col 3): expected `)` : have end of file"));
        fclose(NXT.inp);
    }
    {
        init(NULL, stropen("r", 0, "(\n( \n"));
        Expr e = parser_expr();
        assert(e.sub == EXPR_ERR); assert(!strcmp(e.err.msg, "(ln 1, col 2): expected `)` : have new line"));
        fclose(NXT.inp);
    }
    // EXPR_VAR
    {
        init(NULL, stropen("r", 0, "x:"));
        Expr e = parser_expr();
        assert(e.sub == EXPR_VAR);
        fclose(NXT.inp);
    }
    {
        init(NULL, stropen("r", 0, "x("));
        Expr e = parser_expr();
        assert(e.sub == EXPR_ERR);
        assert(!strcmp(e.err.msg, "(ln 1, col 3): expected `)` : have end of file"));
        fclose(NXT.inp);
    }
    // EXPR_SET
    {
        init(NULL, stropen("r", 0, "set () = (x"));
        Expr e = parser_expr();
        assert(e.sub == EXPR_ERR);
        assert(!strcmp(e.err.msg, "(ln 1, col 5): expected variable : have `(`"));
        fclose(NXT.inp);
    }
    {
        init(NULL, stropen("r", 0, "set a = (x"));
        Expr e = parser_expr();
        assert(e.sub == EXPR_ERR);
        assert(!strcmp(e.err.msg, "(ln 1, col 11): expected `)` : have end of file"));
        fclose(NXT.inp);
    }
    {
        init(NULL, stropen("r", 0, "set a = (x)"));
        Expr e = parser_expr();
        assert(e.sub == EXPR_SET);
        assert(!strcmp(e.Set.var.val.s, "a"));
        assert(!strcmp(e.Set.expr->tk.val.s, "x"));
        fclose(NXT.inp);
    }
    // EXPR_FUNC
    {
        init(NULL, stropen("r", 0, "func :: () ()"));
        assert(parser_expr().sub == EXPR_FUNC);
        fclose(NXT.inp);
    }
    // EXPR_CALL
    {
        init(NULL, stropen("r", 0, "xxx (  )"));
        Expr e = parser_expr();
        assert(e.sub == EXPR_CALL);
        assert(e.Call.func->sub == EXPR_VAR);
        assert(!strcmp(e.Call.func->tk.val.s, "xxx"));
        assert(e.Call.expr->sub == EXPR_UNIT);
        fclose(NXT.inp);
    }
    // EXPR_EXPRS
    {
        init(NULL, stropen("r", 0, ": x"));
        Expr e = parser_expr();
        assert(!strcmp(e.err.msg, "(ln 1, col 3): unexpected indentation level"));
        fclose(NXT.inp);
    }
    {
        init(NULL, stropen("r", 0, ":\nx"));
        Expr e = parser_expr();
        assert(!strcmp(e.err.msg, "(ln 1, col 2): unexpected indentation level"));
        fclose(NXT.inp);
    }
    {
        init(NULL, stropen("r", 0, ":\n    x"));
        Expr e = parser_expr();
        assert(e.sub == EXPR_EXPRS);
        assert(e.exprs.size == 1);
        fclose(NXT.inp);
    }
    {
        init(NULL, stropen("r", 0, ":\n    x x"));
        Expr e = parser_expr();
        assert(!strcmp(e.err.msg, "(ln 2, col 7): expected new line : have `x`"));
        fclose(NXT.inp);
    }
    {
        init(NULL, stropen("r", 0, ":\n    x\n    y ("));
        Expr e = parser_expr();
        assert(!strcmp(e.err.msg, "(ln 3, col 8): expected `)` : have end of file"));
        fclose(NXT.inp);
    }
    {
        init(NULL, stropen("r", 0, ":\n    x\n    y y"));
        Expr e = parser_expr();
        assert(!strcmp(e.err.msg, "(ln 3, col 7): expected new line : have `y`"));
        fclose(NXT.inp);
    }
    {
        init(NULL, stropen("r", 0,
            ":\n"
            "    x\n"
            "    :\n"
            "        y\n"
        ));
        Expr e = parser_expr();
        //dump_expr(e,0);
        assert(e.sub == EXPR_EXPRS);
        assert(e.exprs.size == 2);
        assert(!strcmp(e.exprs.vec[1].exprs.vec[0].tk.val.s, "y"));
        fclose(NXT.inp);
    }
}

void t_parser_decls (void) {
    {
        init(NULL, stropen("r", 0, "a"));
        Decls ds = parser_decls();
        assert(ds.sub == DECLS_ERR);
        assert(!strcmp(ds.err.msg, "(ln 1, col 1): expected `:` : have `a`"));
        fclose(NXT.inp);
    }
    {
        init(NULL, stropen("r", 0, ":\n    a"));
        Decls ds = parser_decls();
        assert(ds.sub == DECLS_ERR);
        assert(!strcmp(ds.err.msg, "(ln 2, col 6): expected `::` : have end of file"));
        fclose(NXT.inp);
    }
    {
//puts(">>>");
//puts(ds.err.msg);
        init(NULL, stropen("r", 0, ":\n    a :: ()"));
        Decls ds = parser_decls();
        assert(ds.sub == DECLS_OK);
        assert(ds.size == 1);
        assert(!strcmp(ds.vec[0].var.val.s, "a"));
        assert(ds.vec[0].type.sub == TYPE_UNIT);
        fclose(NXT.inp);
    }
    {
        init(NULL, stropen("r", 0, ":\n    a :: (x)"));
        Decls ds = parser_decls();
        assert(ds.sub == DECLS_ERR);
        assert(!strcmp(ds.err.msg, "(ln 2, col 11): unexpected `x`"));
        fclose(NXT.inp);
    }
}

void t_parser_block (void) {
    {
        init(NULL, stropen("r", 0, "a:\n    a :: ()"));
        Block blk = parser_block();
        assert(blk.sub == BLOCK_OK);
        fclose(NXT.inp);
    }
    {
        init(NULL, stropen("r", 0,
            ":\n"
            "    a\n"
            "    b\n"
            ":\n"
            "    a :: ()\n"
            "    b :: ()\n"
        ));
        Block blk = parser_block();
        assert(blk.sub == BLOCK_OK);
        assert(blk.expr.sub == EXPR_EXPRS);
        assert(blk.expr.exprs.size == 2);
        assert(blk.decls.sub == DECLS_OK);
        assert(blk.decls.size == 2);
        fclose(NXT.inp);
    }
}

void t_parser (void) {
    t_parser_type();
    t_parser_expr();
    t_parser_decls();
    t_parser_block();
}

void t_code (void) {
    {
        char out[256];
        init(stropen("w",sizeof(out),out), NULL);
        Expr e;
        e.sub = EXPR_VAR;
        e.tk.sym = TK_VAR;
        strcpy(e.tk.val.s, "xxx");
        code_expr(0, e, NULL);
        fclose(NXT.out);
        assert(!strcmp(out,"xxx"));
    }
    {
        char out[256];
        init(stropen("w",sizeof(out),out), NULL);
        Expr e;
            e.sub = EXPR_VAR;
            e.tk.sym = TK_VAR;
            strcpy(e.tk.val.s, "xxx");
        Decl d;
            d.var.sym = TK_VAR;
            strcpy(d.var.val.s, "xxx");
            d.type.sub = TYPE_UNIT;
        Decls ds;
            ds.sub = DECLS_OK;
            ds.size = 1;
            ds.vec = &d;
        code_block(0, (Block) { BLOCK_OK, .decls=ds, .expr=e }, "ret");
        fclose(NXT.out);
        //puts(out);
        assert(!strcmp(out,"int /* () */ xxx;\nret = xxx;\n"));
    }
    {
        char out[256];
        init (
            stropen("w", sizeof(out), out),
            stropen("r", 0, "a:\n    a :: ()")
        );
        Block b = parser_block();
        code(b);
        fclose(NXT.out);
        char* ret =
            "#include <stdio.h>\n"
            "int main (void) {\n"
            "    int ret;\n"
            "    int /* () */ a;\n"
            "    ret = a;\n"
            "    printf(\"%d\\n\", ret);\n"
            "}\n";
        assert(!strcmp(out,ret));
    }
    {
        char out[256];
        init (
            stropen("w", sizeof(out), out),
            stropen("r", 0, "a:\n    a :: ()")
        );
        Block b = parser_block();
        code(b);
        fclose(NXT.out);
        compile(out);
        FILE* f = popen("./a.out", "r");
        assert(f != NULL);
        fgets(out, sizeof(out), f);
        fclose(f);
        assert(!strcmp(out,"0\n"));
    }
}

int main (void) {
    t_lexer();
    t_parser();
    t_code();
    puts("OK");
}

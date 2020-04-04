#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "lexer.h"
#include "parser.h"

FILE* stropen (char* str) {
    return fmemopen(str, strlen(str), "r");
}

void t_lexer (void) {
    {
        CUR.buf = stropen("-- foobar");
        assert(lexer().sym == TK_COMMENT);
        assert(lexer().sym == TK_EOF);
        fclose(CUR.buf);
    }
    {
        CUR.buf = stropen("-- c1\n--c2\n\n");
        assert(lexer().sym == TK_COMMENT);
        assert(lexer().sym == TK_LINE);
        assert(lexer().sym == TK_COMMENT);
        assert(lexer().sym == TK_LINE);
        assert(lexer().sym == TK_LINE);
        assert(lexer().sym == TK_EOF);
        fclose(CUR.buf);
    }
    {
        CUR.buf = stropen("\n  \n");
        assert(lexer().sym == TK_NONE);
        assert(lexer().sym == TK_LINE);
        fclose(CUR.buf);
    }
    {
        CUR.buf = stropen("c1\nc2 c3'  \n    \nc4");
        Tk tk1 = lexer(); assert(tk1.sym == TK_VAR); assert(!strcmp(tk1.val.s, "c1"));
        assert(lexer().sym == TK_LINE);
        Tk tk3 = lexer(); assert(tk3.sym == TK_VAR); assert(!strcmp(tk3.val.s, "c2"));
        Tk tk4 = lexer(); assert(tk4.sym == TK_VAR); assert(!strcmp(tk4.val.s, "c3'"));
        assert(lexer().sym == TK_LINE);
        assert(lexer().sym == TK_LINE);
        Tk tk5 = lexer(); assert(tk5.sym == TK_VAR); assert(!strcmp(tk5.val.s, "c4"));
        assert(lexer().sym == TK_EOF);
        fclose(CUR.buf);
    }
    {
        CUR.buf = stropen("c1 C1 C'a a'? C!!");
        Tk tk1 = lexer(); assert(tk1.sym == TK_VAR);  assert(!strcmp(tk1.val.s, "c1"));
        Tk tk2 = lexer(); assert(tk2.sym == TK_DATA); assert(!strcmp(tk2.val.s, "C1"));
        Tk tk3 = lexer(); assert(tk3.sym == TK_DATA); assert(!strcmp(tk3.val.s, "C'a"));
        Tk tk4 = lexer(); assert(tk4.sym == TK_VAR);  assert(!strcmp(tk4.val.s, "a'?"));
        Tk tk5 = lexer(); assert(tk5.sym == TK_DATA); assert(!strcmp(tk5.val.s, "C!!"));
        assert(lexer().sym == TK_EOF);
        fclose(CUR.buf);
    }
    {
        CUR.buf = stropen("let xlet letx");
        assert(lexer().sym == TK_LET);
        Tk tk1 = lexer(); assert(tk1.sym == TK_VAR); assert(!strcmp(tk1.val.s, "xlet"));
        Tk tk2 = lexer(); assert(tk2.sym == TK_VAR); assert(!strcmp(tk2.val.s, "letx"));
        assert(lexer().sym == TK_EOF);
        fclose(CUR.buf);
    }
    {
        CUR.buf = stropen(": :: :");
        assert(lexer().sym == ':');
        assert(lexer().sym == TK_DECL);
        assert(lexer().sym == ':');
        fclose(CUR.buf);
    }
}

void t_parser_type (void) {
    {
        parser_init(stropen("()"));
        assert(parser_type().sub == TYPE_UNIT);
        fclose(CUR.buf);
    }
}

void t_parser_expr (void) {
    // PARENS
    {
        parser_init(stropen("(())"));
        assert(parser_expr().sub == EXPR_UNIT);
        fclose(CUR.buf);
    }
    {
        parser_init(stropen("( ( ) )"));
        assert(parser_expr().sub == EXPR_UNIT);
        fclose(CUR.buf);
    }
    {
        parser_init(stropen("("));
        Expr e = parser_expr();
        assert(e.sub == EXPR_NONE); assert(!strcmp(e.err.msg, "(ln 1, col 2): expected `)` : have end of file"));
        fclose(CUR.buf);
    }
    {
        parser_init(stropen("(("));
        Expr e = parser_expr();
        assert(e.sub == EXPR_NONE); assert(!strcmp(e.err.msg, "(ln 1, col 3): expected `)` : have end of file"));
        fclose(CUR.buf);
    }
    {
        parser_init(stropen("(\n( \n"));
        Expr e = parser_expr();
        assert(e.sub == EXPR_NONE); assert(!strcmp(e.err.msg, "(ln 1, col 2): expected `)` : have new line"));
        fclose(CUR.buf);
    }
    // EXPR_SET
    {
        parser_init(stropen("set () = (x"));
        Expr e = parser_expr();
        assert(e.sub == EXPR_NONE);
        assert(!strcmp(e.err.msg, "(ln 1, col 5): expected variable : have `(`"));
        fclose(CUR.buf);
    }
    {
        parser_init(stropen("set a = (x"));
        Expr e = parser_expr();
        assert(e.sub == EXPR_NONE);
        assert(!strcmp(e.err.msg, "(ln 1, col 11): expected `)` : have end of file"));
        fclose(CUR.buf);
    }
    {
        parser_init(stropen("set a = (x)"));
        Expr e = parser_expr();
        assert(e.sub == EXPR_SET);
        assert(!strcmp(e.Set.var.val.s, "a"));
        assert(!strcmp(e.Set.expr->tk.val.s, "x"));
        fclose(CUR.buf);
    }
    // EXPR_FUNC
    {
        parser_init(stropen("func :: () ()"));
        assert(parser_expr().sub == EXPR_FUNC);
        fclose(CUR.buf);
    }
    // EXPR_CALL
    {
        parser_init(stropen("xxx (  )"));
        Expr e = parser_expr();
        assert(e.sub == EXPR_CALL);
        assert(e.Call.func->sub == EXPR_VAR);
        assert(!strcmp(e.Call.func->tk.val.s, "xxx"));
        assert(e.Call.expr->sub == EXPR_UNIT);
        fclose(CUR.buf);
    }
    // EXPR_EXPRS
    {
        parser_init(stropen(": x"));
        Expr e = parser_expr();
        assert(!strcmp(e.err.msg, "(ln 1, col 3): unexpected indentation level"));
        fclose(CUR.buf);
    }
    {
        parser_init(stropen(":\nx"));
        Expr e = parser_expr();
        assert(!strcmp(e.err.msg, "(ln 1, col 2): unexpected indentation level"));
        fclose(CUR.buf);
    }
    {
        parser_init(stropen(":\n    x"));
        Expr e = parser_expr();
        assert(e.sub == EXPR_EXPRS);
        assert(e.exprs.size == 1);
        fclose(CUR.buf);
    }
    {
        parser_init(stropen(
            ":\n"
            "    x\n"
            "    :\n"
            "        y\n"
        ));
        Expr e = parser_expr();
        //parser_dump_expr(e,0);
        assert(e.sub == EXPR_EXPRS);
        assert(e.exprs.size == 2);
        assert(!strcmp(e.exprs.vec[1].exprs.vec[0].tk.val.s, "y"));
        fclose(CUR.buf);
    }
}

void t_parser_decls (void) {
    {
        parser_init(stropen("a"));
        Decls ds = parser_decls();
        assert(ds.sub == DECLS_NONE);
        assert(!strcmp(ds.err.msg, "(ln 1, col 2): expected `::` : have end of file"));
        fclose(CUR.buf);
    }
#if 0
    {
        parser_init(stropen("a :: ()"));
        Decls ds = parser_decls();
        assert(ds.var.sym  == TK_VAR);
        assert(!strcmp(ds.var.val.s, "a"));
        assert(ds.type.sub == TYPE_UNIT);
        fclose(CUR.buf);
    }
#endif
    {
        parser_init(stropen("a :: (x)"));
        Decls ds = parser_decls();
        assert(ds.sub == DECLS_NONE);
//puts(ds.err.msg);
        assert(!strcmp(ds.err.msg, "(ln 1, col 7): unexpected `x`"));
        fclose(CUR.buf);
    }
}

void t_parser (void) {
    t_parser_type();
    t_parser_expr();
    t_parser_decls();
}

int main (void) {
    t_lexer();
    t_parser();
    puts("OK");
}

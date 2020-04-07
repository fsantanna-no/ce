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

int all (const char* xp, char* src) {
    static char out[65000];
    init (
        stropen("w", sizeof(out), out),
        stropen("r", 0, src)
    );
    Prog prog;
    if (!parser_prog(&prog)) {
        puts(ALL.err);
        return !strcmp(ALL.err, xp);
    }
    code(prog);
    fclose(ALL.out);
puts(out);
    compile(out);
    FILE* f = popen("./a.out", "r");
    assert(f != NULL);
    fgets(out, sizeof(out), f);
    fclose(f);
    return !strcmp(out,xp);
}

void t_lexer (void) {
    {
        ALL.inp = stropen("r", 0, "-- foobar");
        assert(lexer().sym == TK_COMMENT);
        assert(lexer().sym == TK_EOF);
        fclose(ALL.inp);
    }
    {
        ALL.inp = stropen("r", 0, "-- c1\n--c2\n\n");
        assert(lexer().sym == TK_COMMENT);
        assert(lexer().sym == TK_LINE);
        assert(lexer().sym == TK_COMMENT);
        assert(lexer().sym == TK_LINE);
        assert(lexer().sym == TK_LINE);
        assert(lexer().sym == TK_EOF);
        fclose(ALL.inp);
    }
    {
        ALL.inp = stropen("r", 0, "\n  \n");
        assert(lexer().sym == TK_ERR);
        assert(lexer().sym == TK_LINE);
        fclose(ALL.inp);
    }
    {
        ALL.inp = stropen("r", 0, "c1\nc2 c3'  \n    \nc4");
        Tk tk1 = lexer(); assert(tk1.sym == TK_IDVAR); assert(!strcmp(tk1.val.s, "c1"));
        assert(lexer().sym == TK_LINE);
        Tk tk3 = lexer(); assert(tk3.sym == TK_IDVAR); assert(!strcmp(tk3.val.s, "c2"));
        Tk tk4 = lexer(); assert(tk4.sym == TK_IDVAR); assert(!strcmp(tk4.val.s, "c3'"));
        assert(lexer().sym == TK_LINE);
        assert(lexer().sym == TK_LINE);
        Tk tk5 = lexer(); assert(tk5.sym == TK_IDVAR); assert(!strcmp(tk5.val.s, "c4"));
        assert(lexer().sym == TK_EOF);
        fclose(ALL.inp);
    }
    {
        ALL.inp = stropen("r", 0, "c1 C1 C'a a'? C!!");
        Tk tk1 = lexer(); assert(tk1.sym == TK_IDVAR);  assert(!strcmp(tk1.val.s, "c1"));
        Tk tk2 = lexer(); assert(tk2.sym == TK_IDDATA); assert(!strcmp(tk2.val.s, "C1"));
        Tk tk3 = lexer(); assert(tk3.sym == TK_IDDATA); assert(!strcmp(tk3.val.s, "C'a"));
        Tk tk4 = lexer(); assert(tk4.sym == TK_IDVAR);  assert(!strcmp(tk4.val.s, "a'?"));
        Tk tk5 = lexer(); assert(tk5.sym == TK_IDDATA); assert(!strcmp(tk5.val.s, "C!!"));
        assert(lexer().sym == TK_EOF);
        fclose(ALL.inp);
    }
    {
        ALL.inp = stropen("r", 0, "let xlet letx");
        assert(lexer().sym == TK_LET);
        Tk tk1 = lexer(); assert(tk1.sym == TK_IDVAR); assert(!strcmp(tk1.val.s, "xlet"));
        Tk tk2 = lexer(); assert(tk2.sym == TK_IDVAR); assert(!strcmp(tk2.val.s, "letx"));
        assert(lexer().sym == TK_EOF);
        fclose(ALL.inp);
    }
    {
        ALL.inp = stropen("r", 0, ": :: :");
        assert(lexer().sym == ':');
        assert(lexer().sym == TK_DECL);
        assert(lexer().sym == ':');
        fclose(ALL.inp);
    }
}

void t_parser_type (void) {
    {
        init(NULL, stropen("r", 0, "()"));
        Type tp;
        parser_type(&tp);
        assert(tp.sub == TYPE_UNIT);
        fclose(ALL.inp);
    }
}

void t_parser_datas (void) {
    {
        init(NULL, stropen("r", 0, "data Err"));
        Data dts;
        assert(!parser_data(&dts));
        assert(!strcmp(ALL.err, "(ln 1, col 9): expected `=` or `:` : have end of file"));
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0, "data Km = ()"));
        Data dts;
        parser_data(&dts);
        assert(dts.size == 1);
        assert(!strcmp(dts.tk.val.s,"Km"));
        assert(dts.vec[0].idx == 0);
        assert(dts.vec[0].type.sub == TYPE_UNIT);
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0, "data Bool:\n    False = ()\n    True = ()"));
        Data dts;
        parser_data(&dts);
        assert(dts.size == 2);
        assert(dts.vec[0].idx == 0);
        assert(dts.vec[1].idx == 1);
        assert(!strcmp(dts.vec[0].tk.val.s,"False"));
        assert(!strcmp(dts.vec[1].tk.val.s,"True"));
        assert(dts.vec[0].type.sub == TYPE_UNIT);
        assert(dts.vec[1].type.sub == TYPE_UNIT);
        fclose(ALL.inp);
    }
#if 0
    // TODO: both at same time
    {
        init(NULL, stropen("r", 0, "data Ast = Int:\n    Expr = Int\n    Decl = ()"));
        Data dts;
        parser_data(&dts);
        assert(dts.size == 2);
        assert(dts.vec[1].idx == 1);
        assert(!strcmp(dts.vec[0].tk.val.s,"Data"));
        assert(dts.vec[1].type.sub == TYPE_UNIT);
        assert(!strcmp(dts.vec[1].type.tk.val.s,"True"));
        fclose(ALL.inp);
    }
#endif
}

void t_parser_expr (void) {
    // PARENS
    {
        init(NULL, stropen("r", 0, "(())"));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0, "( ( ) )"));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0, "("));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 1, col 2): expected expression : have end of file"));
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0, "(("));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 1, col 3): expected expression : have end of file"));
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0, "(\n( \n"));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 1, col 2): expected expression : have new line"));
        fclose(ALL.inp);
    }
    // EXPR_VAR
    {
        init(NULL, stropen("r", 0, "x)"));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_VAR);
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0, "x("));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 1, col 3): expected expression : have end of file"));
        fclose(ALL.inp);
    }
    // EXPR_SET
    {
        init(NULL, stropen("r", 0, "set () = (x"));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 1, col 5): expected variable : have `(`"));
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0, "set a = (x"));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 1, col 11): expected `)` : have end of file"));
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0, "set a = (x)"));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_SET);
        assert(!strcmp(e.Set.var.val.s, "a"));
        assert(!strcmp(e.Set.val->Var.val.s, "x"));
        fclose(ALL.inp);
    }
    // EXPR_FUNC
    {
        init(NULL, stropen("r", 0, "func :: () ()"));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_FUNC);
        fclose(ALL.inp);
    }
    // EXPR_CALL
    {
        init(NULL, stropen("r", 0, "xxx (  )"));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_CALL);
        assert(e.Call.func->sub == EXPR_VAR);
        assert(!strcmp(e.Call.func->Var.val.s, "xxx"));
        assert(e.Call.arg->sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
    // EXPR_SEQ
    {
        init(NULL, stropen("r", 0, ": x"));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 1, col 3): unexpected indentation level"));
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0, ":\nx"));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 1, col 2): unexpected indentation level"));
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0, ":\n    x"));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_SEQ);
        assert(e.Seq.size == 1);
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0, ":\n    x x"));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 2, col 7): expected new line : have `x`"));
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0, ":\n    x\n    y ("));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 3, col 8): expected expression : have end of file"));
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0, ":\n    x\n    y y"));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 3, col 7): expected new line : have `y`"));
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0,
            ":\n"
            "    x\n"
            "    :\n"
            "        y\n"
        ));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_SEQ);
        assert(e.Seq.size == 2);
        assert(!strcmp(e.Seq.vec[1].Seq.vec[0].Var.val.s, "y"));
        fclose(ALL.inp);
    }
}

void t_parser_decls (void) {
    {
        init(NULL, stropen("r", 0, "a"));
        Decls ds;
        assert(!parser_decls(&ds));
        assert(!strcmp(ALL.err, "(ln 1, col 1): expected `:` : have `a`"));
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0, ":\n    var a"));
        Decls ds;
        assert(!parser_decls(&ds));
        assert(!strcmp(ALL.err, "(ln 2, col 10): expected `::` : have end of file"));
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0, ":\n    var a :: ()"));
        Decls ds;
        assert(parser_decls(&ds));
        assert(ds.size == 1);
        assert(!strcmp(ds.vec[0].var.val.s, "a"));
        assert(ds.vec[0].type.sub == TYPE_UNIT);
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0, ":\n    var a :: (x)"));
        Decls ds;
        assert(!parser_decls(&ds));
        assert(!strcmp(ALL.err, "(ln 2, col 15): expected type : have `x`"));
        fclose(ALL.inp);
    }
}

void t_parser_block (void) {
    {
        init(NULL, stropen("r", 0, "a:\n    var a :: ()"));
        Expr blk;
        assert(parser_expr(&blk));
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0,
            ":\n"
            "    a where:\n"
            "        var a :: ()\n"
            "    b where:\n"
            "        var b :: ()\n"
        ));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_SEQ);
        assert(e.Seq.size == 2);
        assert(e.Seq.vec[0].sub == EXPR_BLOCK);
        assert(e.Seq.vec[1].sub == EXPR_BLOCK);
        assert(e.Seq.vec[0].Block.decls->size == 1);
        assert(e.Seq.vec[1].Block.decls->size == 1);
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0,
            "x where:\n"
            "    var x :: () = y where:\n"
            "        var y :: ()\n"
            "    var a :: ()\n"
        ));
        Expr e;
        assert(parser_expr(&e));
        assert(e.Block.ret->sub == EXPR_VAR);
        assert(e.Block.decls->size == 2);
        fclose(ALL.inp);
    }
}

void t_parser (void) {
    t_parser_type();
    t_parser_datas();
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
        e.Var.sym = TK_IDVAR;
        strcpy(e.Var.val.s, "xxx");
        code_expr(0, e, NULL);
        fclose(ALL.out);
        assert(!strcmp(out,"xxx"));
    }
    {
        char out[256];
        init(stropen("w",sizeof(out),out), NULL);
        Expr e;
            e.sub = EXPR_VAR;
            e.Var.sym = TK_IDVAR;
            strcpy(e.Var.val.s, "xxx");
        Decl d;
            d.set = NULL;
            d.var.sym = TK_IDVAR;
            strcpy(d.var.val.s, "xxx");
            d.type.sub = TYPE_UNIT;
        Decls ds = { 1, &d };
        Expr blk = { EXPR_BLOCK, .Block={&e,&ds} };
        // xxx: xxx::()
        tce_ret ret = { "ret", NULL };
        code_expr(0, blk, &ret);
        fclose(ALL.out);
        assert(!strcmp(out,"int /* () */ xxx;\nret = xxx"));
    }
    {
        char out[256];
        init (
            stropen("w", sizeof(out), out),
            stropen("r", 0, ":\n    var a :: ()\n    show(a)")
        );
        Prog p;
        parser_prog(&p);
        code(p);
        fclose(ALL.out);
        char* ret =
            "#include \"ce.c\"\n"
            "int main (void) {\n"
            "\n"
            "int /* () */ a;\n"
            "show(a);\n"
            "\n"
            "}\n";
        assert(!strcmp(out,ret));
    }
    {
        char out[1024];
        init (
            stropen("w", sizeof(out), out),
            stropen("r", 0,
                "data Bool:\n"
                "    False = ()\n"
                "    True = ()"
            )
        );
        Data d;
        parser_data(&d);
        code_data(d);
        fclose(ALL.out);
        char* ret =
            "#define SUP_False Bool_False\n"
            "#define False ((Bool) { Bool_False })\n"
            "#define SUP_True Bool_True\n"
            "#define True ((Bool) { Bool_True })\n"
            "\n"
            "typedef enum {\n"
            "    Bool_False,\n"
            "    Bool_True\n"
            "} BOOL;\n"
            "\n"
            "typedef struct Bool {\n"
            "    BOOL sub;\n"
            "    union {\n"
            "    };\n"
            "} Bool;\n\n";
        assert(!strcmp(out,ret));
    }
}

void t_all (void) {
    assert(all(
        "0\n",
        ":\n    var a :: ()\n    set a = ()\n    show(a)"
    ));
    assert(all(
        "1\n",
        ":\n"
        "    data Bool:\n"
        "        False = ()\n"
        "        True  = ()\n"
        "    show(toint(True))"
    ));
    assert(all(
        "1\n",
        ":\n"
        "    data Bool:\n"
        "        False = ()\n"
        "        True  = ()\n"
        "    var v :: Bool\n"
        "    set v = case ():\n"
        "        ()   -> True\n"
        "        else -> True\n"
        "    show(toint(v))"
    ));
    assert(all(
        "1\n",
        ":\n"
        "    data Bool:\n"
        "        False = ()\n"
        "        True  = ()\n"
        "    set inv = func :: Bool -> Bool :\n"
        "        case ...:\n"
        "            False -> True\n"
        "            True  -> False\n"
        "    show(toint(inv(False)))"
    ));
    assert(all(
        "1\n",
        ":\n"
        "    data Bool:\n"
        "        False = ()\n"
        "        True  = ()\n"
        "    data Vv = Bool\n"
        "    var v :: Vv = Vv(True)\n"
        "    var b :: Bool = case v:\n"
        "        Vv(False) -> False\n"
        "        Vv(=x)    -> x where:\n"
        "            var x :: Bool\n"
        "    show(toint(b))"
    ));
}

int main (void) {
    t_lexer();
    t_parser();
    t_code();
    t_all();
    puts("OK");
}

#include "all.h"

int all (const char* xp, char* src) {
    static char out[65000];
    all_init (
        stropen("w", sizeof(out), out),
        stropen("r", 0, src)
    );
    Prog prog;
    if (!parser_prog(&prog)) {
        puts(ALL.err);
        return !strcmp(ALL.err, xp);
    }
    code(prog);
    fclose(ALL.out[OGLOB]);
puts(out);
    remove("a.out");

    // compile
    {
        FILE* f = popen("gcc -xc -", "w");
        assert(f != NULL);
        fputs(out, f);
        fclose(f);
    }

    // execute
    {
        FILE* f = popen("./a.out", "r");
        assert(f != NULL);
        char* cur = out;
        int n = sizeof(out) - 1;
        while (1) {
            char* ret = fgets(cur,n,f);
            if (ret == NULL) {
                break;
            }
            n -= strlen(ret);
            cur += strlen(ret);
        }
    }
/*
puts(">>>");
puts(out);
puts("---");
puts(xp);
puts("<<<");
*/
    return !strcmp(out,xp);
}

void t_lexer (void) {
    {
        ALL.inp = stropen("r", 0, "-- foobar");
        assert(lexer().sym == TK_COMMENT);
        assert(lexer().sym == '\n');
        assert(lexer().sym == EOF);
        fclose(ALL.inp);
    }
    {
        ALL.inp = stropen("r", 0, "-- c1\n--c2\n\n");
        assert(lexer().sym == TK_COMMENT);
        assert(lexer().sym == '\n');
        assert(lexer().sym == TK_COMMENT);
        assert(lexer().sym == '\n');
        assert(lexer().sym == '\n');
        assert(lexer().sym == EOF);
        fclose(ALL.inp);
    }
    {
        ALL.inp = stropen("r", 0, "\n  \n");
        Tk tk1 = lexer(); assert(tk1.sym == '\n'); assert(tk1.val.n == 2);
        Tk tk2 = lexer(); assert(tk2.sym == '\n'); assert(tk2.val.n == 0);
        assert(lexer().sym == EOF);
        fclose(ALL.inp);
    }
    {
        ALL.inp = stropen("r", 0, "c1\nc2 c3'  \n    \nc4");
        Tk tk1 = lexer(); assert(tk1.sym == TK_IDVAR); assert(!strcmp(tk1.val.s, "c1"));
        assert(lexer().sym == '\n');
        Tk tk3 = lexer(); assert(tk3.sym == TK_IDVAR); assert(!strcmp(tk3.val.s, "c2"));
        Tk tk4 = lexer(); assert(tk4.sym == TK_IDVAR); assert(!strcmp(tk4.val.s, "c3'"));
        assert(lexer().sym == '\n');
        assert(lexer().sym == '\n');
        Tk tk5 = lexer(); assert(tk5.sym == TK_IDVAR); assert(!strcmp(tk5.val.s, "c4"));
        assert(lexer().sym == '\n');
        assert(lexer().sym == EOF);
        fclose(ALL.inp);
    }
    {
        ALL.inp = stropen("r", 0, "c1 C1 C'a a'? C!!");
        Tk tk1 = lexer(); assert(tk1.sym == TK_IDVAR);  assert(!strcmp(tk1.val.s, "c1"));
        Tk tk2 = lexer(); assert(tk2.sym == TK_IDDATA); assert(!strcmp(tk2.val.s, "C1"));
        Tk tk3 = lexer(); assert(tk3.sym == TK_IDDATA); assert(!strcmp(tk3.val.s, "C'a"));
        Tk tk4 = lexer(); assert(tk4.sym == TK_IDVAR);  assert(!strcmp(tk4.val.s, "a'?"));
        Tk tk5 = lexer(); assert(tk5.sym == TK_IDDATA); assert(!strcmp(tk5.val.s, "C!!"));
        assert(lexer().sym == '\n');
        assert(lexer().sym == EOF);
        fclose(ALL.inp);
    }
    {
        ALL.inp = stropen("r", 0, "let xlet letx");
        assert(lexer().sym == TK_LET);
        Tk tk1 = lexer(); assert(tk1.sym == TK_IDVAR); assert(!strcmp(tk1.val.s, "xlet"));
        Tk tk2 = lexer(); assert(tk2.sym == TK_IDVAR); assert(!strcmp(tk2.val.s, "letx"));
        assert(lexer().sym == '\n');
        assert(lexer().sym == EOF);
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
        all_init(NULL, stropen("r", 0, "()"));
        Type tp;
        parser_type(&tp);
        assert(tp.sub == TYPE_UNIT);
        fclose(ALL.inp);
    }
}

void t_parser_datas (void) {
    {
        all_init(NULL, stropen("r", 0, "data Err"));
        Data dts;
        assert(parser_data(&dts));
        assert(ALL.data_recs.size == 1);
        //assert(!strcmp(ALL.err, "(ln 1, col 9): expected `=` or `:` : have end of file"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "data Km ()"));
        Data dts;
        parser_data(&dts);
        assert(dts.size == 1);
        assert(!strcmp(dts.tk.val.s,"Km"));
        assert(dts.vec[0].idx == 0);
        assert(dts.vec[0].type.sub == TYPE_UNIT);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "data Bool:\n    False ()\n    True ()"));
        Data dts;
        parser_data(&dts);
        assert(lexer().sym == EOF);
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
        all_init(NULL, stropen("r", 0, "data Ast (Int):\n    Expr Int\n    Decl ()"));
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
        all_init(NULL, stropen("r", 0, "(())"));
        Env* env = NULL;
        Expr e;
        assert(parser_expr(&env,&e));
        assert(e.sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "( ( ) )"));
        Env* env = NULL;
        Expr e;
        assert(parser_expr(&env,&e));
        assert(e.sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "("));
        Env* env = NULL;
        Expr e;
        assert(!parser_expr(&env,&e));
        assert(!strcmp(ALL.err, "(ln 1, col 2): expected expression : have end of line"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "(("));
        Env* env = NULL;
        Expr e;
        assert(!parser_expr(&env,&e));
        assert(!strcmp(ALL.err, "(ln 1, col 3): expected expression : have end of line"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "(\n( \n"));
        Env* env = NULL;
        Expr e;
        assert(!parser_expr(&env,&e));
        assert(!strcmp(ALL.err, "(ln 1, col 2): expected expression : have end of line"));
        fclose(ALL.inp);
    }
    // EXPR_VAR
    {
        all_init(NULL, stropen("r", 0, "x)"));
        Env* env = NULL;
        Expr e;
        assert(parser_expr(&env,&e));
        assert(e.sub == EXPR_VAR);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "x("));
        Env* env = NULL;
        Expr e;
        assert(!parser_expr(&env,&e));
        assert(!strcmp(ALL.err, "(ln 1, col 3): expected expression : have end of line"));
        fclose(ALL.inp);
    }
    // EXPR_SET
    {
        all_init(NULL, stropen("r", 0, "set () = (x"));
        Env* env = NULL;
        Expr e;
        assert(!parser_expr(&env,&e));
        assert(!strcmp(ALL.err, "(ln 1, col 12): expected `)` : have end of line"));
        //assert(!strcmp(ALL.err, "(ln 1, col 5): expected variable : have `(`"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "set a = (x"));
        Env* env = NULL;
        Expr e;
        assert(!parser_expr(&env,&e));
        assert(!strcmp(ALL.err, "(ln 1, col 11): expected `)` : have end of line"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "set a = (x)"));
        Env* env = NULL;
        Expr e;
        assert(parser_expr(&env,&e));
        assert(e.sub == EXPR_SET);
        assert(!strcmp(e.Set.patt.Set.val.s, "a"));
        assert(!strcmp(e.Set.expr->Var.val.s, "x"));
        fclose(ALL.inp);
    }
    // EXPR_FUNC
    {
        all_init(NULL, stropen("r", 0, "func :: () ()"));
        Env* env = NULL;
        Expr e;
        assert(parser_expr(&env,&e));
        assert(e.sub == EXPR_FUNC);
        fclose(ALL.inp);
    }
    // EXPR_CALL
    {
        all_init(NULL, stropen("r", 0, "xxx (  )"));
        Env* env = NULL;
        Expr e;
        assert(parser_expr(&env,&e));
        assert(e.sub == EXPR_CALL);
        assert(e.Call.func->sub == EXPR_VAR);
        assert(!strcmp(e.Call.func->Var.val.s, "xxx"));
        assert(e.Call.arg->sub == EXPR_UNIT);
        fclose(ALL.inp);
    }
    // EXPR_SEQ
    {
        all_init(NULL, stropen("r", 0, ": x"));
        Env* env = NULL;
        Expr e;
        assert(!parser_expr(&env,&e));
        assert(!strcmp(ALL.err, "(ln 1, col 3): expected new line : have `x`"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, ":\nx"));
        Env* env = NULL;
        Expr e;
        assert(!parser_expr(&env,&e));
        assert(!strcmp(ALL.err, "(ln 2, col 1): expected indentation of 4 spaces : have `x`"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, ":\n    x"));
        Env* env = NULL;
        Expr e;
        assert(parser_expr(&env,&e));
        assert(e.sub == EXPR_SEQ);
        assert(e.Seq.size == 1);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, ":\n    x x"));
        Env* env = NULL;
        Expr e;
        assert(!parser_expr(&env,&e));
        assert(!strcmp(ALL.err, "(ln 2, col 7): expected new line : have `x`"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, ":\n    x\n    y ("));
        Env* env = NULL;
        Expr e;
        assert(!parser_expr(&env,&e));
        assert(!strcmp(ALL.err, "(ln 3, col 8): expected expression : have end of line"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, ":\n    x\n    y ("));
        Env* env = NULL;
        Expr e;
        assert(!parser_expr(&env,&e));
        assert(!strcmp(ALL.err, "(ln 3, col 8): expected expression : have end of line"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, ":\n    x\n    y y"));
        Env* env = NULL;
        Expr e;
        assert(!parser_expr(&env,&e));
        assert(!strcmp(ALL.err, "(ln 3, col 7): expected new line : have `y`"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, "val ::"));
        Env* env = NULL;
        Expr e;
        assert(!parser_expr(&env,&e));
        assert(!strcmp(ALL.err, "(ln 1, col 5): expected variable identifier : have `::`"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0,
            ":\n"
            "    x\n"
            "    :\n"
            "        y\n"
            "    z\n"
        ));
        Env* env = NULL;
        Expr e;
        assert(parser_expr(&env,&e));
        assert(e.sub == EXPR_SEQ);
        assert(e.Seq.size == 3);
        assert(!strcmp(e.Seq.vec[1].Seq.vec[0].Var.val.s, "y"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0,
            ":\n"
            "    x\n"
            "    :\n"
            "        y\n"
        ));
        Env* env = NULL;
        Expr e;
        assert(parser_expr(&env,&e));
        assert(e.sub == EXPR_SEQ);
        assert(e.Seq.size == 2);
        assert(!strcmp(e.Seq.vec[1].Seq.vec[0].Var.val.s, "y"));
        fclose(ALL.inp);
    }
}

void t_parser_decls (void) {
    Env* env = NULL;
    int ll_accept1 (TK tk);
    int err_expected (const char* v);
    int parser_decl (Env** env, Decl* decl);
    typedef struct {
        int   size;
        Decl* vec;
    } Decls;
    int xxx (Decl* decl) {
        if (!ll_accept1(TK_MUT) && !ll_accept1(TK_VAL) && !ll_accept1(TK_FUNC)) {
            return err_expected("`mut` or `val` or `func`");
        }
        return parser_decl(&env,decl);
    }
    void* parser_decl_ (Env** env) {
        static Decl d_;
        Decl d;
        if (!xxx(&d)) {
            return NULL;
        }
        d_ = d;
        return &d_;
    }
    int parser_decls (Decls* ret) {
        List lst;
        if (!parser_list_line(NULL, 1, &lst, &parser_decl_, sizeof(Decl))) {
            return 0;
        }
        *ret = (Decls) { lst.size, lst.vec };
        return 1;
    }

    {
        all_init(NULL, stropen("r", 0, "a"));
        Decls ds;
        assert(!parser_decls(&ds));
        assert(!strcmp(ALL.err, "(ln 1, col 1): expected `:` : have `a`"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, ":\n    val a"));
        Decls ds;
        assert(!parser_decls(&ds));
        assert(!strcmp(ALL.err, "(ln 2, col 10): expected `::` : have end of line"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, ":\n    val a :: ()"));
        Decls ds;
        assert(parser_decls(&ds));
        assert(ds.size == 1);
        assert(!strcmp(ds.vec[0].patt.Set.val.s, "a"));
        assert(ds.vec[0].type.sub == TYPE_UNIT);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, ":\n    val a :: {char}"));
        Decls ds;
        assert(parser_decls(&ds));
        assert(ds.size == 1);
        assert(!strcmp(ds.vec[0].patt.Set.val.s, "a"));
        assert(ds.vec[0].type.sub == TYPE_RAW);
        assert(!strcmp(ds.vec[0].type.Raw.val.s,"char"));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, ":\n    val (a,b) :: ((),{char})\n    val x :: ()"));
        Decls ds;
        assert(parser_decls(&ds));
        assert(ds.size == 2);
        assert(!strcmp(ds.vec[0].patt.Tuple.vec[0].Set.val.s, "a"));
        assert(ds.vec[0].type.sub == TYPE_TUPLE);
        assert(!strcmp(ds.vec[0].type.Tuple.vec[1].Raw.val.s,"char"));
        assert(ds.vec[1].type.sub == TYPE_UNIT);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0, ":\n    val a :: (x)"));
        Decls ds;
        assert(!parser_decls(&ds));
        assert(!strcmp(ALL.err, "(ln 2, col 15): expected type : have `x`"));
        fclose(ALL.inp);
    }
}

void t_parser_block (void) {
    {
        all_init(NULL, stropen("r", 0, "a:\n    val a :: ()"));
        Env* env = NULL;
        Expr blk;
        assert(parser_expr(&env,&blk));
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0,
            ":\n"
            "    a where:\n"
            "        val a :: ()\n"
            "    b where:\n"
            "        val b :: ()\n"
        ));
        Env* env = NULL;
        Expr e;
        assert(parser_expr(&env,&e));
        assert(e.sub == EXPR_SEQ);
        assert(e.Seq.size == 2);
        assert(e.Seq.vec[0].where != NULL);
        assert(e.Seq.vec[1].where != NULL);
        assert(e.Seq.vec[0].where->Seq.vec[0].sub == EXPR_DECL);
        assert(e.Seq.vec[1].where->Seq.vec[0].sub == EXPR_DECL);
        fclose(ALL.inp);
    }
    {
        all_init(NULL, stropen("r", 0,
            "x where:\n"
            "    val x :: () = y where:\n"
            "        val y :: ()\n"
            "    val a :: ()\n"
        ));
        Env* env = NULL;
        Expr e;
        assert(parser_expr(&env,&e));
        assert(e.sub == EXPR_VAR);
        assert(e.where != NULL);
        assert(e.where->sub == EXPR_SEQ);
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
        all_init(stropen("w",sizeof(out),out), NULL);
        Env env = { ENV_PLAIN, NULL, .Plain={{TK_IDVAR,{.s="xxx"}},{}} };
        Expr e = { EXPR_VAR, {}, &env, NULL, {} };
            e.Var.sym = TK_IDVAR;
            strcpy(e.Var.val.s, "xxx");
        code_expr(e, NULL);
        fclose(ALL.out[OGLOB]);
        assert(!strcmp(out,"xxx"));
    }
    {
        char out[256] = "";
        all_init(stropen("w",sizeof(out),out), NULL);
        Env env = { ENV_PLAIN, NULL, .Plain={{TK_IDVAR,{.s="xxx"}},{}} };
        Expr e = { EXPR_VAR, {}, &env, NULL, {} };
            e.Var.sym = TK_IDVAR;
            strcpy(e.Var.val.s, "xxx");
        Decl d;
            d.init = NULL;
            d.patt.sub = PATT_SET;
            d.patt.Set.sym = TK_IDVAR;
            strcpy(d.patt.Set.val.s, "xxx");
            d.type.sub = TYPE_UNIT;
        Expr n = { EXPR_DECL, {}, NULL, NULL, .Decl=d };
        e.where = &n;
        // xxx: xxx::()
        tce_ret ret = { {{TK_IDVAR,{.s="ret"}},{TYPE_NONE}}, NULL };
        code_expr(e, &ret);
        fclose(ALL.out[OGLOB]);
        assert(!strcmp(out,"{\nint xxx;\nret = xxx;\n}\n"));
    }
    {
        char out[256] = "";
        all_init (
            stropen("w", sizeof(out), out),
            stropen("r", 0, "val a :: ()\n{show_Bool}(a)")
        );
        Prog p;
        parser_prog(&p);
        code(p);
        fclose(ALL.out[OGLOB]);
        char* ret =
            "#include \"inc/ce.c\"\n"
            "int main (void) {\n"
            "\n"
            //"#line 1\n"
            "int a;\n"
            ";\n"
            //"#line 2\n"
            "show_Bool(a);\n"
            "\n"
            "}\n";
        assert(!strcmp(out,ret));
    }
    {
        char out[1024] = "";
        all_init (
            stropen("w", sizeof(out), out),
            stropen("r", 0, "val inp :: ()\nmatch {fgetc} (inp):\n    {'\\n'} -> ()")
        );
        Prog p;
        parser_prog(&p);
        code(p);
        fclose(ALL.out[OGLOB]);
        char* ret =
            "#include \"inc/ce.c\"\n"
            "int main (void) {\n"
            "\n"
            "int inp;\n"
            ";\n"
            //"#line 1\n"
            "{\n"
            "typeof(fgetc(inp)) ce_tst = fgetc(inp);\n"
            "if (ce_tst == '\\n') {\n"
            ";\n"
            "1;\n"
            "} else {\n"
            "}\n"
            "}\n"
            ";\n"
            "\n"
            "}\n";
puts(out);
        assert(!strcmp(out,ret));
    }
    {
        char out[1024];
        all_init (
            stropen("w", sizeof(out), out),
            stropen("r", 0,
                "data Bool:\n"
                "    False ()\n"
                "    True ()"
            )
        );
        Data d;
        parser_data(&d);
        code_data(d);
        fclose(ALL.out[OGLOB]);
        char* xp =
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
            "} Bool;\n"
            "\n"
            "void show_Bool (Bool v) {\n"
            "    switch (v.sub) {\n"
            "        case Bool_False:\n"
            "            puts(\"False\");\n"
            "            break;\n"
            "        case Bool_True:\n"
            "            puts(\"True\");\n"
            "            break;\n"
            "        default:\n"
            "            assert(0 && \"bug found\");\n"
            "    }\n"
            "}\n"
            "\n";
        assert(!strcmp(out,xp));
    }
}

void t_all (void) {
    assert(all(
        "1\n",
        "mut a :: ()\nset a = ()\n{ printf(\"%d\\n\",a) }\n"
    ));
    assert(all(
        "1\n",
        "let a :: () = ():\n    { printf(\"%d\\n\",a) }\n"
    ));
    assert(all(
        "99\n",
        "val b :: () = match {99}:\n    a :: {int} -> a\n{ printf(\"%d\\n\",b) }"
    ));
    assert(all(
        "99\n",
        "val b :: () = match {99}:\n    a :: {int}:\n        {1}\n        a\n{ printf(\"%d\\n\",b) }"
    ));
    assert(all(
        "99\n",
        "val b :: () = let a :: {int} = {99}:\n    {1}\n    a\n{ printf(\"%d\\n\",b) }"
    ));
    assert(all(
        "99\n",
        "val b :: () = let it :: {int} = {99} -> it\n{ printf(\"%d\\n\",b) }\n"
    ));
    assert(all(
        "99\n",
        "val b :: () = let it :: {int} = {99} -> it\n{ printf(\"%d\\n\",b) }\n"
    ));

    // ENV

    assert(all(
        "()\n",
        "i where:\n"
        "    val i :: () = ()\n"
        "{show_Unit}()\n"
    ));
    assert(all(
        "()\n",
        "{show_Unit}(i) where:\n"
        "    val i :: () = ()\n"
    ));
#if 0
    // TODO: i not declared
    assert(all(
        "()\n",
        ":\n"
        "    val i :: () = ()\n"
        "{show_Unit}(i)\n"
    ));
    // TODO: i not declared
    assert(all(
        "()\n",
        "{show_Unit}(i) where:\n"
        "    val i :: () = ()\n"
        "{show_Unit}(i)"
    ));
#endif

    // TODO: multiple vars
    assert(all(
        "()\n",
        "val (a,b) :: ((),()) = ((),())\n{show_Unit}(b)\n"
    ));
    assert(all(
        "99\n",
        "let (a,b) :: ((),{int}) = ((),{99}):\n    { printf(\"%d\\n\",b) }\n"
    ));
    assert(all(
        "True\n",
        "data Bool:\n"
        "    False\n"
        "    True\n"
        "{show_Bool}(True)\n"
    ));
    assert(all(
        "True\n",
        "data List\n"
        "data Bool:\n"
        "    False ()\n"
        "    True  ()\n"
        "{show_Bool}(True)\n"
    ));
    assert(all(
        "True\n",
        "data List\n"
        "data Bool:\n"
        "    False ()\n"
        "    True  ()\n"
        "mut x :: Bool = False\n"
        "{x = True}\n"
        "{show_Bool}(x)\n"
    ));
    assert(all(
        "()\n",
        "val v :: () = match ():\n"
        "    ~() -> ()\n"
        "{show_Unit}(v)"
    ));
    assert(all(
        "()\n",
        "val a :: () = ()\n"
        "val b :: () = match ():\n"
        "    ~a -> ()\n"
        "{show_Unit}(b)"
    ));
    assert(all(
        "()\n",
        "val a :: () = ()\n"
        "val b :: {int} = a ~ ()\n"
        "val c :: () = if b -> () -> {99}\n"
        "{show_Unit}(c)"
    ));
    assert(all(
        "()\n",
        "val a :: () = ()\n"
        "val c :: () = if:\n"
        "    a ~ () -> ()\n"
        "    else   -> {99}\n"
        "{show_Unit}(c)"
    ));
    assert(all(
        "()\n",
        "val a :: () = ()\n"
        "{show_Unit}(if a~() -> () -> {99})"
    ));
    assert(all(
        "False\n",
        "data Bool:\n"
        "    False ()\n"
        "    True  ()\n"
        "mut v :: Bool\n"
        "set v = match ():\n"
        "    ()   -> False\n"
        "    else -> True\n"
        "{show_Bool}(v)"
    ));
    assert(all(
        "10",
        "data Bool:\n"
        "    False ()\n"
        "    True  ()\n"
        "val v :: {int} = {10}\n"
        "val r :: {int} = {0} ~ v\n"
        "{printf(\"%d\",v)}"
    ));
    assert(all(
        "False\n",
        "data Bool:\n"
        "    False ()\n"
        "    True  ()\n"
        "func inv :: (Bool -> Bool):\n"
        "    match ...:\n"
        "        False -> True\n"
        "        True  -> False\n"
        "{show_Bool}(inv(True))"
    ));
    assert(all(
        "False\n",
        "data Bool:\n"
        "    False ()\n"
        "    True  ()\n"
        "func inv :: (Bool -> Bool):\n"
        "    match ...:\n"
        "        False -> return True\n"
        "        True  -> return False\n"
        "{show_Bool}(inv(True))"
    ));
    assert(all(
        "True\n",
        "data Bool:\n"
        "    False ()\n"
        "    True  ()\n"
        "data Vv Bool\n"
        "val v :: Vv = Vv(True)\n"
        "val b :: Bool = match v:\n"
        "    Vv(False)     -> False\n"
        "    Vv(x) :: Bool -> x\n"
        "{show_Bool}(b)"
    ));
    assert(all(
        "()\n",
        "val i :: ((),()) = ((),())\n"
        "match i:\n"
        "    (x,_) :: () -> {show_Unit}(x)"
    ));
    assert(all(
        "()\n",
        "match i:\n"
        "    (x,_) :: () -> {show_Unit}(x)\n"
        "where:\n"
        "    val i :: ((),()) = ((),())\n"
    ));
    assert(all(
        "()\n",
        "match i:\n"
        "    (x,_) :: ():\n"
        "        {show_Unit}(x)\n"
        "where:\n"
        "    val i :: ((),()) = ((),())\n"
    ));
    assert(all(
        "()\n",
        ":\n"
        "    match i:\n"
        "        (x,_) :: () -> {show_Unit}(x)\n"
        "    where:\n"
        "        val i :: ((),()) = ((),())\n"
    ));
    assert(all(
        "()\n",
        ":\n"
        "    ()\n"
        "({show_Unit})()\n"
    ));
    assert(all(
        "()\n",
        ":\n"
        "    match i:\n"
        "        (x,_) :: () -> {show_Unit}(x)\n"
        "    where:\n"
        "        val i :: ((),()) = ((),())\n"
        "()\n"
    ));
    assert(all(
        "()\n",
        ":\n"
        "    match i:\n"
        "        () -> ()\n"
        "    where:\n"
        "        val i :: () = ()\n"
        "    match i:\n"
        "        (x,_) :: () -> {show_Unit}(x)\n"
        "    where:\n"
        "        val i :: ((),()) = ((),())\n"
    ));
    assert(all(
        "()\n",
        "val i :: ((),((),())) = ((),((),()))\n"
        "match i:\n"
        "    (_,(x,_)) :: () -> {show_Unit}(x)"
    ));
    assert(all(
        "()\n",
        "val i :: ((),((),())) = ((),((),()))\n"
        "val j :: ((),((),())) = i\n"
        "val v :: () = match j:\n"
        "    ((),x) :: ((),()) -> y where:\n"
        "        val y :: () = match x:\n"
        "            ((),z) :: () -> z\n"
        "{show_Unit}(v)"
    ));
    assert(all(
        "()\n",
        "val i :: ((),((),())) = ((),((),()))\n"
        "val j :: ((),((),())) = i\n"
        "val v :: () = match j:\n"
        "    (x,(_,z)) :: ((),()) -> y where:\n"
        "        val y :: () = match (x,z):\n"
        "            ((),a) :: () -> a\n"
        "{show_Unit}(v)"
    ));
    assert(all(
        "()\n",
        "data Pair ((),())\n"
        "val n :: () = match Pair ((),()):\n"
        "    Pair (x,_) :: () -> x\n"
        "{show_Unit}(n)"
    ));
    assert(all(
        "True\n",
        "data Bool:\n"
        "    False ()\n"
        "    True  ()\n"
        "data Pair (Bool,Bool)\n"
        "val n :: Bool = match Pair (True,False):\n"
        "    Pair (x,_) :: Bool -> x\n"
        "{show_Bool}(n)"
    ));

    // DATA - LIST - RECURSIVE

    assert(all(
        "Nil\n",
        "data List\n"
        "data List:\n"
        "    Nil  ()\n"
        "    Cons ((), List)\n"
        "val l :: List[] = new Nil\n"
        "{show_List}(l)"
    ));
    assert(all(
        "Nil\n",
        "data List\n"
        "data List:\n"
        "    Nil  ()\n"
        "    Cons ((), List)\n"
        "func fff :: (() -> List[]):\n"
        "    new Nil\n"
        "val l :: List[] = fff()\n"
        "{show_List}(l)"
    ));
puts("===========");
    assert(all(
        "Cons\n()\n",
        "data List\n"
        "data List:\n"
        "    Nil  ()\n"
        "    Cons ((), List)\n"
        "val l :: List[] = Nil\n"
        "val n :: () = match l:\n"
        "    Nil -> ()\n"
        "{show_List}(l)\n"
        "{show_Unit}(n)"
    ));
assert(0);
    assert(all(
        "Cons\n()\n",
        "data List\n"
        "data List:\n"
        "    Nil  ()\n"
        "    Cons ((), List)\n"
        "val l :: List[] = Nil\n"
        "set l = new Cons((),l)\n"
        "val n :: () = match l:\n"
        "    Cons(x,_) :: () -> x\n"
        "{show_List}(l)\n"
        "{show_Unit}(n)"
    ));
    assert(all(
        "()\n",
        "func f :: (((),()) -> ()) ()\n"
        "{show_Unit}(f((),()))"
    ));
    assert(all(
        "()\n",
        "val x :: () = :\n"
        "    ()\n"
        "    ()\n"
        "{show_Unit}(x)\n"
    ));
#if 0
    // TODO: decl as expr
    assert(all(
        "()\n",
        "val x :: () = :\n"
        "    val y :: () = ()\n"
        "    val z :: () = ()\n"
        "    z\n"
        "{show_Unit}(x)\n"
    ));
    // TODO: nested Cons pattern
    assert(all(
        "()\n",
        "data List\n"
        "data List:\n"
        "    Nil  ()\n"
        "    Cons ((), List)\n"
        "val l :: List = new Cons((),Cons((),Nil))\n"
        "val n :: () = match l:\n"
        "    Cons(_,Cons(x,_)) :: () -> x\n"   // TODO
        "{show_Unit}(n)"
    ));
#endif
    assert(all(
        "Tre\n",
        "data Nat:\n"
        "    One ()\n"
        "    Two ()\n"
        "    Tre ()\n"
        "data List\n"
        "data List:\n"
        "    Nil  ()\n"
        "    Cons (Nat, List)\n"
        "val l :: List = new Cons(Tre,Cons(Two,Cons(One,Nil)))\n"
        "val n :: Nat = match l:\n"
        "    Cons(x,_) :: Nat -> x\n"
        "{show_Nat}(n)"
    ));

    // LOOP

    assert(all(
        "()\n",
        "val n :: () = loop break ()\n"
        "{show_Unit}(n)"
    ));
    assert(all(
        "()\n",
        "loop break ()\n"
        "{show_Unit}(())"
    ));
    assert(all(
        "()\n",
        "loop break () if {1}\n"
        "{show_Unit}(())"
    ));
#if 0
    // TODO: last stmt assigns to loop
    assert(all(
        "()\n",
        "val x :: {int*} = loop:\n"
        "    break {NULL} if {1}\n"
        "    {1.1}\n"
        "{show_Unit}(())"
    ));
    assert(all(
        "()\n",
        "func f :: (()->{int*}):\n"
        "    loop:\n"
        "        break {NULL} if {1}\n"
        "        {1.1}\n"
        "{show_Unit}(())"
    ));
#endif
}

int main (void) {
#if 0
    assert(all(
        "Cons\n()\n",
        "data List\n"
        "data List:\n"
        "    Nil  ()\n"
        "    Cons ((), List)\n"
        "val l :: List[] = Nil\n"
        "val n :: () = match l:\n"
        "    Nil -> ()\n"
        "{show_List}(l)\n"
        "{show_Unit}(n)"
    ));
assert(0);
#endif
    t_lexer();
    t_parser();
    t_code();
    t_all();
    puts("OK");
}

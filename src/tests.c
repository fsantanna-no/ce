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
        assert(lexer().sym == '\n');
        Tk spc = lexer(); assert(spc.sym==' '); assert(spc.val.n == 2);
        assert(lexer().sym == '\n');
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
        assert(lexer().sym == ' ');
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
        assert(parser_data(&dts));
        assert(ALL.data_recs.size == 1);
        //assert(!strcmp(ALL.err, "(ln 1, col 9): expected `=` or `:` : have end of file"));
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0, "data Km ()"));
        Data dts;
        parser_data(&dts);
        assert(dts.size == 1);
        assert(!strcmp(dts.tk.val.s,"Km"));
        assert(dts.vec[0].idx == 0);
        assert(dts.vec[0].type.sub == TYPE_UNIT);
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0, "data Bool:\n    False ()\n    True ()"));
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
        init(NULL, stropen("r", 0, "data Ast (Int):\n    Expr Int\n    Decl ()"));
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
        assert(!strcmp(ALL.err, "(ln 1, col 2): expected expression : have end of line"));
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0, "(("));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 1, col 3): expected expression : have end of line"));
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0, "(\n( \n"));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 1, col 2): expected expression : have end of line"));
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
        assert(!strcmp(ALL.err, "(ln 1, col 3): expected expression : have end of line"));
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
        assert(!strcmp(ALL.err, "(ln 1, col 11): expected `)` : have end of line"));
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
        assert(!strcmp(ALL.err, "(ln 1, col 3): expected new line : have `x`"));
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0, ":\nx"));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 2, col 1): expected indentation of 4 spaces : have `x`"));
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
        assert(!strcmp(ALL.err, "(ln 2, col 7): expected indentation of 4 spaces : have `x`"));
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0, ":\n    x\n    y ("));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 3, col 8): expected expression : have end of line"));
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0, ":\n    x\n    y ("));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 3, col 8): expected expression : have end of line"));
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0, ":\n    x\n    y y"));
        Expr e;
        assert(!parser_expr(&e));
        assert(!strcmp(ALL.err, "(ln 3, col 7): expected indentation of 4 spaces : have `y`"));
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0,
            ":\n"
            "    x\n"
            "    :\n"
            "        y\n"
            "    z\n"
        ));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_SEQ);
        assert(e.Seq.size == 3);
        assert(!strcmp(e.Seq.vec[1].Seq.vec[0].Var.val.s, "y"));
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
        init(NULL, stropen("r", 0, ":\n    val a"));
        Decls ds;
        assert(!parser_decls(&ds));
        assert(!strcmp(ALL.err, "(ln 2, col 10): expected `::` : have end of line"));
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0, ":\n    val a :: ()"));
        Decls ds;
        assert(parser_decls(&ds));
        assert(ds.size == 1);
        assert(!strcmp(ds.vec[0].patt.Set.val.s, "a"));
        assert(ds.vec[0].type.sub == TYPE_UNIT);
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0, ":\n    val a :: {char}"));
        Decls ds;
        assert(parser_decls(&ds));
        assert(ds.size == 1);
        assert(!strcmp(ds.vec[0].patt.Set.val.s, "a"));
        assert(ds.vec[0].type.sub == TYPE_RAW);
        assert(!strcmp(ds.vec[0].type.Raw.val.s,"char"));
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0, ":\n    val (a,b) :: ((),{char})\n    val x :: ()"));
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
        init(NULL, stropen("r", 0, ":\n    val a :: (x)"));
        Decls ds;
        assert(!parser_decls(&ds));
        assert(!strcmp(ALL.err, "(ln 2, col 15): expected type : have `x`"));
        fclose(ALL.inp);
    }
}

void t_parser_block (void) {
    {
        init(NULL, stropen("r", 0, "a:\n    val a :: ()"));
        Expr blk;
        assert(parser_expr(&blk));
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0,
            ":\n"
            "    a where:\n"
            "        val a :: ()\n"
            "    b where:\n"
            "        val b :: ()\n"
        ));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_SEQ);
        assert(e.Seq.size == 2);
        assert(e.Seq.vec[0].decls.size > 0);
        assert(e.Seq.vec[1].decls.size > 0);
        assert(e.Seq.vec[0].decls.size == 1);
        assert(e.Seq.vec[1].decls.size == 1);
        fclose(ALL.inp);
    }
    {
        init(NULL, stropen("r", 0,
            "x where:\n"
            "    val x :: () = y where:\n"
            "        val y :: ()\n"
            "    val a :: ()\n"
        ));
        Expr e;
        assert(parser_expr(&e));
        assert(e.sub == EXPR_VAR);
        assert(e.decls.size == 2);
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
        Expr e = { EXPR_VAR, {.size=0}, {} };
            e.Var.sym = TK_IDVAR;
            strcpy(e.Var.val.s, "xxx");
        code_expr(e, NULL);
        fclose(ALL.out);
        assert(!strcmp(out,"xxx"));
    }
    {
        char out[256] = "";
        init(stropen("w",sizeof(out),out), NULL);
        Expr e = { EXPR_VAR, {.size=0}, {} };
            e.Var.sym = TK_IDVAR;
            strcpy(e.Var.val.s, "xxx");
        Decl d;
            d.init = NULL;
            d.patt.sub = PATT_SET;
            d.patt.Set.sym = TK_IDVAR;
            strcpy(d.patt.Set.val.s, "xxx");
            d.type.sub = TYPE_UNIT;
        e.decls = (Decls) { 1, &d };
        // xxx: xxx::()
        Patt pt = (Patt){PATT_SET,.Set={TK_IDVAR,{.s="ret"}}};
        tce_ret ret = { &pt, NULL };
        code_expr(e, &ret);
        fclose(ALL.out);
        assert(!strcmp(out,"{\nint xxx;\nret = xxx;\n}\n"));
    }
    {
        char out[256] = "";
        init (
            stropen("w", sizeof(out), out),
            stropen("r", 0, "val a :: ()\n{show_Bool}(a)")
        );
        Prog p;
        parser_prog(&p);
        code(p);
        fclose(ALL.out);
        char* ret =
            "#include \"inc/ce.c\"\n"
            "int main (void) {\n"
            "\n"
            "int a;\n"
            "show_Bool(a);\n"
            "\n"
            "}\n";
        assert(!strcmp(out,ret));
    }
    {
        char out[1024] = "";
        init (
            stropen("w", sizeof(out), out),
            stropen("r", 0, "match {fgetc} (inp):\n    {'\\n'} -> ()")
        );
        Prog p;
        parser_prog(&p);
        code(p);
        fclose(ALL.out);
        char* ret =
            "#include \"inc/ce.c\"\n"
            "int main (void) {\n"
            "\n"
            "{\n"
            "typeof(fgetc(inp)) ce_tst = fgetc(inp);\n"
            "if (ce_tst == '\\n') {\n"
            "1;\n"
            "} else {\n"
            "assert(0 && \"match failed\");\n"
            "}\n"
            "}\n"
            ";\n"
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
                "    False ()\n"
                "    True ()"
            )
        );
        Data d;
        parser_data(&d);
        code_data(d);
        fclose(ALL.out);
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
#if 0
    // TODO: multiple vars
    assert(all(
        "()\n",
        "val (a,b) :: ((),()) = ((),())\n{show_Unit}(b)\n"
    ));
    assert(all(
        "99\n",
        "let (a,b) :: ((),{int}) = ((),{99}):\n    { printf(\"%d\\n\",b) }\n"
    ));
#endif
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
#if 0
    // TODO: \n where
    assert(all(
        "()\n",
        "match i:\n"
        "    (x,_) :: () -> {show_Unit}(x)\n"
        "where:\n"
        "    val i :: ((),()) = ((),())\n"
    ));
#endif
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
    assert(all(
        "Nil\n",
        "data List\n"
        "data List:\n"
        "    Nil  ()\n"
        "    Cons ((), List)\n"
        "val l :: List = new Nil\n"
        "{show_List}(l)"
    ));
    assert(all(
        "Cons\n()\n",
        "data List\n"
        "data List:\n"
        "    Nil  ()\n"
        "    Cons ((), List)\n"
        "val l :: List = new Cons((),new Nil)\n"
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
#if 0
    // TODO: nested Cons pattern
    assert(all(
        "()\n",
        "data List\n"
        "data List:\n"
        "    Nil  ()\n"
        "    Cons ((), List)\n"
        "val l :: List = new Cons((),new Cons((),new Nil))\n"
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
        "val l :: List = new Cons(Tre,new Cons(Two,new Cons(One,new Nil)))\n"
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
        "loop break\n"
        "{show_Unit}(())"
    ));
}

int main (void) {
    t_lexer();
    t_parser();
    t_code();
    t_all();
    puts("OK");
}

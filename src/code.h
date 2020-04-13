typedef struct tce_ret {
    Patt* patt;
    struct tce_ret* nxt;
} tce_ret;

void code_data  (Data data);
void code_decls (Decls ds);
void code_expr  (Expr e, tce_ret* ret);
void code_prog  (Prog prog);

void code (Prog prog);
void compile (const char* inp);     // TODO: should pipe TOK1.out -> GCC

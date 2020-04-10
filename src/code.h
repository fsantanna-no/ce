typedef struct tce_ret {
    Patt* patt;
    struct tce_ret* nxt;
} tce_ret;

void code_data  (Data data);
void code_decls (int spc, Decls ds);
void code_expr  (int spc, Expr e, tce_ret* ret);
void code_prog  (int spc, Prog prog);

void code (Prog prog);
void compile (const char* inp);     // TODO: should pipe NXT.out -> GCC

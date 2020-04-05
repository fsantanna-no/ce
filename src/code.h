void code_expr  (int spc, Expr e, const char* ret);
void code_block (int spc, Block blk, const char* ret);
void code_prog  (int spc, Prog prog);

void code (Prog prog);
void compile (const char* inp);     // TODO: should pipe NXT.out -> GCC

void code_expr  (int spc, Expr e, const char* ret);
void code_block (int spc, Block blk, const char* ret);

void code (Block blk);
void compile (const char* inp);     // TODO: should pipe NXT.out -> GCC
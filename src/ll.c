#include "all.h"

void ll_read (State_Tok* tok) {
    tok->off = ftell(ALL.inp);
    tok->tk  = lexer();

    // skip comment
    if (tok->tk.sym == TK_COMMENT) {
        tok->off = ftell(ALL.inp);
        tok->tk  = lexer();
    }
}

void ll_lincol (void) {
    if (TOK1.tk.sym == '\n') {
        TOK2.lin = TOK1.lin + 1;
        TOK2.col = (TOK2.off - TOK1.off);
    } else {
        TOK2.col = TOK1.col + (TOK2.off - TOK1.off);
    }
}

void ll_next () {
    TOK0 = TOK1;
    TOK1 = TOK2;
    ll_read(&TOK2);
    ll_lincol();
}

int ll_accept1 (TK tk) {
    if (TOK1.tk.sym==tk) {
        ll_next();
        return 1;
    } else {
        return 0;
    }
}

#if 0
int ll_accept2 (TK tk, int ok) {
    if (TOK2.tk.sym==tk && ok) {
        ll_next();
        ll_next();
        return 1;
    } else {
        return 0;
    }
}
#endif

int ll_check0 (TK tk) {
    return (TOK0.tk.sym == tk);
}

int ll_check1 (TK tk) {
    return (TOK1.tk.sym == tk);
}

#if 0
int ll_check2 (TK tk) {
    return (TOK2.tk.sym == tk);
}
#endif

#include <stdio.h>

typedef enum {
    TK_ERR = 0,

    // all single-char tokens
    TK_CHAR = 256,

    TK_EOF,
    TK_COMMENT,
    TK_LINE,

    TK_DECL,

    TK_IDVAR,
    TK_IDDATA,

    // all reserved keywords
    TK_RESERVED,
    TK_DATA, TK_FUNC, TK_LET, TK_SET
} TK;

typedef union {
        int  n;
        char s[256];
} TK_val;

typedef struct {
    TK     sym;
    TK_val val;
} Tk;

typedef struct {
    FILE* out;
    FILE* inp;
    char  err[256];
    int   ind;
    long  off;   // position before token (to fallback)
    long  lin;   // line before token
    long  col;   // column before token
    Tk    tk;
} State;

extern State NXT, PRV;

const char* lexer_tk2str (Tk* tk);
int lexer_tk2len (Tk* tk);
Tk lexer ();

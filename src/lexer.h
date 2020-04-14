#include <stdio.h>

typedef enum {
    TK_ERR = 0,

    // all single-char tokens
    TK_CHAR = 256,

    TK_COMMENT,
    TK_RAW,

    TK_DECL,
    TK_ARROW,
    TK_ARG,

    TK_IDVAR,
    TK_IDDATA,

    // all reserved keywords
    TK_RESERVED,
    TK_BREAK, TK_CASE, TK_DATA, TK_ELSE,   TK_FUNC, TK_IF,  TK_LET, TK_LOOP,
    TK_MUT,   TK_NEW,  TK_PASS, TK_RETURN, TK_SET,  TK_VAL, TK_WHERE
} TK;

typedef union {
        int  n;
        char s[256];
} TK_val;

typedef struct {
    TK     sym;
    TK_val val;
} Tk;

enum { OGLOB, ODECL, OEXPR };
typedef struct {
    FILE* inp;
    FILE* out[3];
    char  err[256];
    int   ind;
    struct {
        int size;
        Tk  buf[256];
    } data_recs;
} State_All;

typedef struct {
    long  off;   // position before token (to fallback)
    long  lin;   // line before token
    long  col;   // column before token
    Tk    tk;
} State_Tok;

extern State_All ALL;
extern State_Tok TOK0, TOK1, TOK2;

const char* lexer_tk2str (Tk* tk);
int lexer_tk2len (Tk* tk);
Tk lexer ();

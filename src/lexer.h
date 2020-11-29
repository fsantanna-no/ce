#include <stdio.h>

typedef enum {
    TK_ERR = 0,

    // all single-char tokens
    TK_TOK = 256,

    TK_COMMENT,
    TK_RAW,

    TK_SEQ1,
    TK_DECL,
    TK_ARROW,
    TK_ARG,

    TK_IDVAR,   // 263
    TK_IDDATA,
    TK_CHAR,

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

const char* lexer_tk2str (Tk* tk);
Tk lexer ();

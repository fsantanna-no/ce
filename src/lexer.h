typedef enum {
    TK_NONE = 0,

    // all single-char tokens
    TK_CHAR = 256,

    TK_EOF,
    TK_COMMENT,
    TK_LINE,

    TK_DECL,
    TK_VAR,
    TK_DATA,

    // all reserved keywords
    TK_RESERVED,
    TK_LET
} TK;

typedef union {
        int  n;
        char s[256];
} TK_val;

typedef struct {
    TK     sym;
    TK_val val;
} Tk;

struct {
    FILE* buf;
} LX;

Tk lexer ();

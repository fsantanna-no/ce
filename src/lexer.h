typedef enum TK {
    TK_NONE = 0,

    // all single-char tokens
    TK_CHAR = 256,

    TK_EOF,
    TK_COMMENT,
    TK_LINE,

    TK_VAR,
    TK_DATA,

    // all reserved keywords
    TK_RESERVED,
    TK_LET
} TK;

struct {
    FILE* buf;
    union {
        int  n;
        char s[255];
    } val;
} LX;

TK lexer ();

typedef enum TK {
    TK_NONE = 0,

    // all single-char tokens
    TK_CHAR = 256,

    TK_EOF,
    TK_COMMENT,
    TK_NEWLINE,

    TK_VAR,
    TK_DATA,

    // all reserved keywords
    TK_RESERVED,
    TK_LET
} TK;

char lexer_value[255];

TK lexer (FILE* buf);

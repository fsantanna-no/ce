typedef enum TK {
    TK_ERR = 0,
    TK_EOF,
    TK_COMMENT,
    TK_NEWLINE,

    TK_VAR, TK_DATA,

    TK_MINUS,

    TK_RESERVED,
    TK_LET
} TK;

char lex_value[255];

TK lex (FILE* buf);

typedef enum EXP {
    EXP_NONE = 0,
    EXP_VAR
} EXP;

EXP parser_expr (FILE* buf);

typedef enum EXP {
    EXP_NONE = 0,
    EXP_UNIT,
    EXP_VAR,
    EXP_CONS
} EXP;

EXP parser_exp (FILE* buf);

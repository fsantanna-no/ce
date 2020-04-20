#include "all.h"

FILE* stropen (const char* mode, size_t size, char* str) {
    size = (size != 0) ? size : strlen(str);
    return fmemopen(str, size, mode);
}

DATA datas_data (const char* v) {
    for (int i=0; i<ALL.data.datas.size; i++) {
        if (!strcmp(ALL.data.datas.buf[i].id, v)) {
            return ALL.data.datas.buf[i].kind;
        }
    }
    return DATA_ERROR;
}

CONS datas_cons (const char* v, char** sup) {
    for (int i=0; i<ALL.data.conss.size; i++) {
        if (!strcmp(ALL.data.conss.buf[i].id, v)) {
            if (sup != NULL) {
                *sup = ALL.data.conss.buf[i].sup;
            }
            return ALL.data.conss.buf[i].kind;
        }
    }
    return CONS_ERROR;
}

void all_init (FILE* out, FILE* inp) {
    static char buf1[65000];
    static char buf2[65000];
    FILE* out1 = stropen("w", sizeof(buf1), buf1);
    FILE* out2 = stropen("w", sizeof(buf2), buf2);
    ALL = (State_All) { inp,{out,out1,out2},{},0,{{0,{}},{0,{}}} };
    if (inp != NULL) {
        parser_init();
    }
}

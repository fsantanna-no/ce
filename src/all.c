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

int data_isrec (Data data) {        // TODO: mutually recursive datas
    int aux (Type type) {
        switch (type.sub) {
            case TYPE_UNIT:
                return 0;
            case TYPE_DATA:
                if (!strcmp(data.tk.val.s,type.Data.tk.val.s)) {
                    return 1;
                }
                return 0;
            case TYPE_TUPLE:
                for (int i=0; i<type.Tuple.size; i++) {
                    if (aux(type.Tuple.vec[i])) {
                        return 1;
                    }
                }
                return 0;
            default:
                assert(0 && "TODO");
        }
    }
    for (int i=0; i<data.size; i++) {
        if (aux(data.vec[i].type)) {
            return 1;
        }
    }
    return 0;
}

int datas_isrec (const char* data) {
    for (int i=0; i<ALL.prog.size; i++) {
        Glob g = ALL.prog.vec[i];
        if (g.sub==GLOB_DATA && !strcmp(data,g.data.tk.val.s)) {
            return data_isrec(g.data);
        }
    }
    return 0;
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

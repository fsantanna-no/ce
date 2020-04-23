#include "all.h"

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

Data* data_get (const char* id) {
    for (int i=0; i<ALL.prog.size; i++) {
        Glob g = ALL.prog.vec[i];
        if (g.sub==GLOB_DATA && !strcmp(id,g.data.tk.val.s)) {
            return &ALL.prog.vec[i].data;
        }
    }
    return NULL;
}

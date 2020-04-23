typedef enum {
    TYPE_NONE,
    TYPE_RAW,
    TYPE_UNIT,
    TYPE_DATA,
    TYPE_FUNC,
    TYPE_TUPLE
} TYPE;

typedef struct Type {
    TYPE sub;
    union {
        Tk Raw;
        struct {
            Tk  tk;
            int size;   // -1 if not pool, 0 if unbounded, n if bounded
        } Data;
        struct {        // TYPE_FUNC
            struct Type* inp;
            struct Type* out;
        } Func;
        struct {        // TYPE_TUPLE
            int size;
            struct Type* vec;
        } Tuple;
    };
} Type;

///////////////////////////////////////////////////////////////////////////////

typedef struct {
    int  idx;
    Tk   tk;
    Type type;
} Cons;

typedef struct Data {
    Tk    tk;
    int   size;     // size=0: recursive pre declaration
    Cons* vec;
} Data;

int data_isrec  (Data data);
int datas_isrec (const char* data);

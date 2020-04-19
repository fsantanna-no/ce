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
        Tk Data;
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

typedef struct {
    int  idx;
    Tk   tk;
    Type type;
} Cons;

typedef struct {
    Tk    tk;
    int   size;     // size=0: recursive pre declaration
    Cons* vec;
} Data;

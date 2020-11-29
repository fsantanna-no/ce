- Recursive Data Type
    - At least one constructor w/ a value of its own type (direct or indirect).
    - The first constructor is Nil with no data.
    - Automatic memory management
    - Restrictions:
        - no cycles
        - no pointers from outside (cannot mix roots)
            - watching?

data List:
    Nil  ()     -- implicit, must be ommited
    Cons List   -- recursive constructor

val l :: List[]   = new Cons(Cons(Nil))
    -- l points to root of data. when it goes out of scope, all data is recursively reclaimed

val l :: List[10] = new Cons(Cons(Nil))
    -- dealloc on scope
    -- pool slot size is the max among constructors
    -- root is a fat pointer

{show_List}(l)

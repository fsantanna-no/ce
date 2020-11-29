# ce

## Lexical Rules

### Symbols

```
    [       ]       (       )       {       }       ,       _
    ~       ?       =       EOF     :       ::      ...     ->
```

### Comments

```
    -- ignore until EOL or EOF
```

### Keywords

```
    break   match   data    else    func    if      let     loop
    mut     new     pass    return  set     val     where
```

### C Code

```
RAW ::= { ... }
```

### Identifiers

```
VAR  ::= [a-z][a-z,A-Z,0-9,_,',?,!]+
DATA ::= [A-Z][a-z,A-Z,0-9,_,',?,!]+
```

## Syntax

```
Prog ::= { Glob }
Glob ::= Data | Decl | Expr

Data ::= data DATA Type `:´ {DATA Type `\n´}

Expr ::= RAW | VAR | DATA | `(´ `)´ | `(´ Expr `)´ | `...´ | `pass´
      |  `return´ Expr
      |  `break´ Expr
      |  `new´ Expr
      |  `set´ Patt `=´ Expr
      |  (`val´ | `mut´) Decl
      |  `func´ (Decl | `::´ Type Expr)
      |  `(´ Expr {`,´ Expr} `)´
      |  `:´ `\n´ {Expr `\n´}
      |  `let´ Decl [`->´] Expr
      |  `if´ `:´ `\n´ { (Expr|`else`) [`->´] Expr `\n´}
      |  `if´ Expr [`->´] Expr [`->´] Expr
      |  `case´ Expr `:´ `\n´ { (Patt|`else`) [Decl] [`->´] Expr `\n´}
      |  `loop´ Expr

Decl ::=

Type ::= RAW | `(´ `)´ | `(´ Type `)´
      |  `(´ Type {`,´ Type} `)´
      |  `(´ Type `->´ Type `)´
```

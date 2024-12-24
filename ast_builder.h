#ifndef AST_BUILDER_H_
#define AST_BUILDER_H_

#include <stddef.h> // size_t
#include <stdint.h> // uint32_t

#include "tokenizer.h"

/*
  @def AST Node Types and Definitions
  
  TOP : (DECL|FUNC)*      -- technically +
      
  FUNC: int IDEN ( [int IDEN] (, int IDEN)* ) BLCK
  BLCK: { stmt* }
  DECL: int IDEN [ LITR ] ... ;
      
  stmt: collectively referring to:
  {
  DECL
  IFEL: if ( EXPR ) stmt|BLCK [else stmt|BLCK]
  WHIL: while ( EXPR ) stmt|BLCK
  RETN: return EXPR ;
  EXPS: EXPR|atom ; // expr statement
  -- note: the only unary operator is `!`
  }

  EXPR: [(] [EXPR|atom] (UPOP|BIOP) EXPR|atom [)]
  -- note: parsed differently from others, mainly because of infix expr recursive definition making the first item an EXPR
  
  atom: atomic expression, collectively referring to:
  {
  VARR: IDEN [[ EXPR ] ...]       -- var or array
  CALL: IDEN ( [EXPR] (, EXPR)* )
  INTG: [SIGN] DECI
  }
  
  UPOP: ...
  BIOP: ...
  SIGN: TOKN that is + or -       -- note that according to HW, !+- same prec
  DECM: TOKN that is [0-9]+       -- decimal literal
  IDEN: TOKN that is [a-zA-Z_][a-zA-Z0-9_]*
*/

extern const char * ast_builder_errmsg;

typedef struct AST_Node {
    uint32_t type;

    // for IDEN, SIGN, DECM, UPOP and BIOP
    Token * token;
    // Symbol * symref; // reference to symbol list item
    
    // children nodes, conforming to DA protocol
    struct AST_Node * items;
    size_t count;
    size_t capacity;
} AST_Node;
/*
typedef struct {
    Token iden;
    AST_Node * def;
} Symbol;

typedef struct {
    Symbol * items;
    size_t count;
    size_t capacity;
} SymbolList;
*/
typedef struct {
    //SymbolList symbols; // vars and funcs
    AST_Node top;
} AST;

// int ast_build(AST * ast, Tokenizer * tok); // return 1 on fail
int ast_build(AST_Node * ast, Tokenizer * tok); // return 1 on fail
// void ast_free(AST * ast);
void ast_free_node(AST_Node * node);
// void ast_print(AST ast);
void ast_print_node(AST_Node node, int indent);

#endif // AST_BUILDER_H_

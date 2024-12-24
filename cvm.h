#ifndef CVM_H_
#define CVM_H_

#include "ast_builder.h"

int cvm_run(int * ret_val, AST_Node * ast, FILE * is_, FILE * os_);

#endif // CVM_H_

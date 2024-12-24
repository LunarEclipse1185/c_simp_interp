#include <stdio.h> // io stream
#include <stddef.h>
#include <string.h>

#include "cvm.h"
#include "dynarray.h"
#include "tokenizer.h"
#include "ast_builder.h"

// @desc
// simplified version:
// no local variable,
// all global variable declarations must
// precede all function definitions

typedef struct {
    size_t * items;
    size_t count;
    size_t capacity;
} Dimensions;

// int or int[]...
typedef struct {
    Token iden;
    Dimensions dims; // empty for int

    int value; // for int
    int * values; // for int array
} Var;

typedef struct {
    Var * items;
    size_t count;
    size_t capacity;
} Vars;

typedef struct {
    Vars * items;
    size_t count;
    size_t capacity;
} CallStack;

typedef struct {
    Token iden;
    AST_Node * def;
} Func;

typedef struct {
    Func * items;
    size_t count;
    size_t capacity;
} Funcs;

static Vars globals = {};
static Funcs funcs = {};
static CallStack callstack = {};

static FILE * is = NULL;
static FILE * os = NULL;


void cvm_cleanup();
int cvm_call(int * ret_val, Func * func, Vars args);
Vars * cvm_callstack_get();
void cvm_callstack_push(Vars newframe);
void cvm_callstack_pop();
int cvm_execute_block(int * ret_val, AST_Node * node);
int cvm_execute_stmt(int * ret_val, AST_Node * node);
int cvm_eval_expr(int * ret_val, AST_Node * node);

int parse_int(Token * tok) {
    int res;
    // @assert the integer literal is followed by a non-digit char
    sscanf(tok->begin, "%d", &res);
    return res;
}
size_t parse_size_t(Token * tok) {
    size_t res;
    // @assert the integer literal is followed by a non-digit char
    sscanf(tok->begin, "%zu", &res);
    return res;
}
#define tokstrcmp(tok, str) (strlen(str) != (tok)->len || strncmp(str, (tok)->begin, (tok)->len))
int tokcmp(Token * l, Token * r) {
    if (l->len != r->len) return 1;
    return strncmp(l->begin, r->begin, l->len);
}

Var * cvm_find_var(Vars * vars, Token * iden) {
    for (size_t i = 0; i < vars->count; ++i) {
        if (tokcmp(iden, &vars->items[i].iden) == 0) return vars->items + i;
    }
    return NULL;
}
Func * cvm_find_func(Token * iden) {
    for (size_t i = 0; i < funcs.count; ++i) {
        if (tokcmp(iden, &funcs.items[i].iden) == 0) return funcs.items + i;
    }
    return NULL;
}

void cvm_cleanup() {
    da_free(&funcs);
    
    for (size_t i = 0; i < globals.count; ++i) {
        free(globals.items[i].values);
        da_free(&globals.items[i].dims);
    }
    da_free(&globals);

    for (size_t i = 0; i < callstack.count; ++i) {
        for (int j = 0; j < callstack.items[i].count; ++j) {
            free(callstack.items[i].items[j].values);
            da_free(&callstack.items[i].items[j].dims);
        }
        da_free(&callstack.items[i]);
    }
    da_free(&callstack);
}


int cvm_run(int * ret_val, AST_Node * ast, FILE * is_, FILE * os_) {
    is = is_;
    os = os_;

    for (AST_Node * node = ast->items; node - ast->items < ast->count; ++node) {
        switch (node->type) {
        case 'DECL': {
            // @assert node->count > 0
            Var newvar = {*node->items[0].token};
            if (node->count > 1) {
                size_t size = 1;
                for (size_t i = 1; i < node->count; ++i) {
                    size_t dim = parse_size_t(node->items[i].token);
                    size *= dim;
                    da_append(&newvar.dims, dim);
                }
                newvar.values = (int *)malloc(size * sizeof(int));
            }
            da_append(&globals, newvar);
        } break;
        case 'FUNC': {
            // @assert node->count > 0
            Func newfunc = {.iden = *node->items[0].token, .def = node};
            /*
            if (node->count > 1) {
                for (size_t i = 1; i < node->count ; ++i) {
                    da_append(&newfunc.params, (Var) {*node->items[0].token});
                }
                }*/
            da_append(&funcs, newfunc);
        } break;
        } // switch
    }

    const char * entry_point_name = "main";
    Token entry_point_token = {entry_point_name, strlen(entry_point_name)};
    Func * entry_point = cvm_find_func(&entry_point_token);
    int status;
    if (entry_point) {
        Vars args = {}; // no argument for main
        status = cvm_call(ret_val, entry_point, args);
    } else {
        status = 1;
    }
    
    cvm_cleanup();
    return status;
}


int cvm_call(int * ret_val, Func * func, Vars args) {
    cvm_callstack_push(args);
    AST_Node * block = func->def->items + func->def->count - 1;
    int status = cvm_execute_block(ret_val, block);
    if (status == 0 && ret_val) *ret_val = 0;
    if (status == 2) status = 0;
    cvm_callstack_pop();
    return status;
}

Vars * cvm_callstack_get() {
    return &callstack.items[callstack.count - 1];
}

void cvm_callstack_push(Vars newframe) {
    da_append(&callstack, newframe);
}

void cvm_callstack_pop() {
    // @assert callstack.count > 0
    callstack.count -= 1;
    Vars * popped = callstack.items + callstack.count;
    for (size_t i = 0; i < popped->count; ++i) {
        free(popped->items[i].values);
        da_free(&popped->items[i].dims);
    }
    da_free(popped);
}

// if no return stmt is encountered, `ret_val` is not modified
// so it is safe to pass NULL
// @return 0 if success (no return), 1 if syntax error, 2 if returned
int cvm_execute_block(int * ret_val, AST_Node * node) {
    int status = 0;
    for (size_t i = 0; i < node->count; ++i) {
        status = cvm_execute_stmt(ret_val, node->items + i);
        if (status) return status; // 1 or 2
    }
    return status; // should be 0
}

// @return 0 if success, 1 if syntax error, 2 if returned
int cvm_execute_stmt(int * ret_val, AST_Node * node) {
    int status = 0;
    switch (node->type) {
    case 'DECL': {
        // @assert node->count > 0
        Var newvar = {*node->items[0].token};
        if (node->count > 1) {
            size_t size = 1;
            for (size_t i = 1; i < node->count; ++i) {
                size_t dim = parse_size_t(node->items[i].token);
                size *= dim;
                da_append(&newvar.dims, dim);
            }
            newvar.values = (int *)malloc(size * sizeof(int));
        }
        Vars * frame = cvm_callstack_get();
        da_append(frame, newvar);
    } break;
    case 'EXPS': {
        // @assert node->count == 1
        status = cvm_eval_expr(NULL, node->items + 0);
        if (status == 2 || status == 3) status = 0; // eval to cin/cout 
    } break;
    case 'IFEL': {
        // @assert node->count == 2 or 3
        int cond;
        status = cvm_eval_expr(&cond, node->items + 0);
        if (status) break;
        
        AST_Node * branch = NULL;
        if (cond) branch = node->items + 1;
        else if (node->count == 3) branch = node->items + 2;
        if (branch == NULL) break;
        if (branch->type == 'BLCK') status = cvm_execute_block(ret_val, branch);
        else status = cvm_execute_stmt(ret_val, branch);
    } break;
    case 'WHIL': {
        // @assert node->count == 2
        int cond;
        while (1) {
            status = cvm_eval_expr(&cond, node->items + 0);
            if (status || !cond) break;
            if (node->items[1].type == 'BLCK') status = cvm_execute_block(ret_val, node->items + 1);
            else status = cvm_execute_stmt(ret_val, node->items + 1);
        }
    } break;
    case 'RETN': {
        // @assert node->count == 1
        status = cvm_eval_expr(ret_val, node->items + 0);
        if (status) break;
        status = 2; // successfully returned
    } break;
    default: status = -1; // @assert unreachable
    }
    return status;
}

int * get_value(AST_Node * node) {
    // @assert node->type == 'VARR'
    Var * var = cvm_find_var(cvm_callstack_get(), node->items[0].token);
    if (!var) {
        var = cvm_find_var(&globals, node->items[0].token);
        if (!var) return NULL;
    }
    if (node->count == 1) { // int
        return &var->value;
    } else { // int array
        // var->dims: int, int
        // node->items: iden intg intg
        if (node->count - 1 != var->dims.count) { // check dimension equal
            return NULL;
        }
        size_t index = 0; // index in 1d-array
        size_t postfix_hypervolume = 1;
        for (int i = var->dims.count - 1; i >= 0; --i) {
            int thisindex;
            int status = cvm_eval_expr(&thisindex, node->items + i + 1);
            if (status) return NULL;
            // [!] @assume thisindex in-bounds
            index += postfix_hypervolume * (size_t)thisindex;
            postfix_hypervolume *= var->dims.items[i];
        }
        return var->values + index;
    }
}


int is_cout(AST_Node * node) {
    return node->count == 1 &&
        node->items[0].token != NULL &&
        tokstrcmp(node->items[0].token, "cout");
}
int is_cin(AST_Node * node) {
    return node->count == 1 &&
        node->items[0].token != NULL &&
        tokstrcmp(node->items[0].token, "cin");
}
int is_endl(AST_Node * node) {
    return node->count == 1 &&
        node->items[0].token != NULL &&
        tokstrcmp(node->items[0].token, "endl");
}
// @return 0 for evaluated to int, 1 for syntax error, 2 for evaluated to cout, 3 for cin
int cvm_eval_expr(int * ret_val, AST_Node * node) {
    // @assert node->type == 'EXPR'
    int status = 0;
    switch (node->type) {
    case 'EXPR': return cvm_eval_expr(ret_val, node->items + 0);
    case 'VARR': {
        int * value = get_value(node);
        if (!value) {
            status = 1;
            break;
        }
        if (ret_val) *ret_val = *value;
    } break;
    case 'CALL': {
        Func * func = cvm_find_func(node->items[0].token);
        // node: iden expr expr .. expr
        // func: iden expr expr .. expr blck
        if (!func || node->count + 1 != func->def->count) {
            status = 1;
            break;
        }
        Vars args = {}; // must be DA of int
        // pass arguments
        for (int i = 1; i < node->count; ++i) {
            int thisarg;
            status = cvm_eval_expr(&thisarg, node->items + i);
            if (status) break;
            da_append(&args, ((Var) {.iden = *func->def->items[i].token, .value = thisarg}));
        }
        status = cvm_call(ret_val, func, args); // `args` ownership passed to stack manager
    } break;
    case 'INTG': {
        if (ret_val) {
            *ret_val = parse_int(node->items[node->count - 1].token);
            if (node->count == 2 &&
                node->items[0].token->begin[0] == '-') {
                *ret_val *= -1;
            }
        }
    } break;
    case 'UPOP': {
        // @assert node->token is "!"
        int val;
        status = cvm_eval_expr(&val, node->items + 0);
        if (status) break;
        if (ret_val) *ret_val = !val;
    } break;
    case 'BIOP': {
        // @assert node->count == 2
        if (tokstrcmp(node->token, "<<") == 0) {
            if (!node->items[0].items[0].token ||
                tokstrcmp(node->items[0].items[0].token, "cout") != 0) {
                if (cvm_eval_expr(NULL, node->items + 0) != 2) { // cout
                    status = 1;
                    break;
                }
            }
            if (node->items[1].items[0].token &&
                tokstrcmp(node->items[1].items[0].token, "endl") == 0) {
                fprintf(os, "\n");
                status = 2; // return cout
                break;
            } 
            int r;
            status = cvm_eval_expr(&r, node->items + 1);
            if (status) break;
            fprintf(os, "%d", r);
            status = 2; // return cout
            break;
        } else if (tokstrcmp(node->token, ">>") == 0) {
            if (!node->items[0].items[0].token ||
                tokstrcmp(node->items[0].items[0].token, "cin") != 0) {
                if (cvm_eval_expr(NULL, node->items + 1) != 3) { // cin
                    status = 1;
                    break;
                }
            }
            int * pr = get_value(node->items + 1);
            if (!pr) {
                status = 1;
                break;
            }
            fscanf(is, "%d", pr);
            status = 3; // return cin
            break;
        } else if (tokstrcmp(node->token, "=") == 0) {
            if (node->items[0].type != 'VARR') {
                status = 1;
                break;
            }
            int * pl = get_value(node->items + 0);
            if (!pl) {
                status = 1;
                break;
            }
            int r;
            status = cvm_eval_expr(&r, node->items + 1);
            if (status) break;
            *pl = r;
            if (ret_val) *ret_val = r;
            break;
        } 

        int l, r;
        status = cvm_eval_expr(&l, node->items + 0);
        if (status) break;
        status = cvm_eval_expr(&r, node->items + 1);
        if (status) break;
        
        if (tokstrcmp(node->token, "*") == 0) {
            if (ret_val) *ret_val = l * r;
        } else if (tokstrcmp(node->token, "/") == 0) {
            if (ret_val) *ret_val = l / r;
        } else if (tokstrcmp(node->token, "%") == 0) {
            if (ret_val) *ret_val = l % r;
        } else if (tokstrcmp(node->token, "+") == 0) {
            if (ret_val) *ret_val = l + r;
        } else if (tokstrcmp(node->token, "-") == 0) {
            if (ret_val) *ret_val = l - r;
        } else if (tokstrcmp(node->token, "<=") == 0) {
            if (ret_val) *ret_val = l <= r;
        } else if (tokstrcmp(node->token, ">=") == 0) {
            if (ret_val) *ret_val = l >= r;
        } else if (tokstrcmp(node->token, "<") == 0) {
            if (ret_val) *ret_val = l < r;
        } else if (tokstrcmp(node->token, ">") == 0) {
            if (ret_val) *ret_val = l > r;
        } else if (tokstrcmp(node->token, "==") == 0) {
            if (ret_val) *ret_val = l == r;
        } else if (tokstrcmp(node->token, "!=") == 0) {
            if (ret_val) *ret_val = l != r;
        } else if (tokstrcmp(node->token, "^") == 0) {
            if (ret_val) *ret_val = l && !r || !l && r;
        } else if (tokstrcmp(node->token, "&&") == 0) {
            if (ret_val) *ret_val = l && r;
        } else if (tokstrcmp(node->token, "||") == 0) {
            if (ret_val) *ret_val = l || r;
        } else {
            // @assert unreachable
            status = -1;
        }
    } break;
    }
    return status;
}

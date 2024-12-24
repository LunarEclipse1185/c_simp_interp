#include <stdio.h> // print
#include <ctype.h> // isalnum
#include <stddef.h>
#include <string.h> // cmp

#include "ast_builder.h"
#include "dynarray.h"
#include "tokenizer.h"

const char * ast_builder_errmsg;

void print_wchar(uint32_t type) {
    for (int i = 3; i >= 0; --i) {
        uint32_t c = type / ((uint32_t)1 << (8 * i)) % ((uint32_t)1 << 8);
        if (c)
            printf("%c", c);
    }
}
void ast_print_node(AST_Node node, int indent)
{
    printf("%*s", indent * 2, "");
    print_wchar(node.type);
    if (node.type == 'SIGN' ||
        node.type == 'DECM' ||
        node.type == 'IDEN' ||
        node.type == 'EXCT') {
        printf(" ( %.*s )\n", (int)node.token->len, node.token->begin);
    } else {
        if (node.type == 'UPOP' ||
            node.type == 'BIOP') {
            printf(" ( %.*s )", (int)node.token->len, node.token->begin);
        }
        printf(" {\n");
        for (size_t i = 0; i < node.count; ++i) {
            ast_print_node(node.items[i], indent + 1);
        }
        printf("%*s}\n", indent * 2, "");
    }
}
/*
void ast_print(AST ast) {
    printf("Variable list: \n    ");
    for (size_t i = 0; i < ast.vars.count; ++i) {
        printf("%.*s%s",
               ast.vars.items[i].iden.len,
               ast.vars.items[i].iden.begin,
               i < ast.vars.count - 1 ? ", " : "\n");
    }
    printf("Function list: \n    ");
    for (size_t i = 0; i < ast.funcs.count; ++i) {
        printf("%.*s%s",
               ast.funcs.items[i].iden.len,
               ast.funcs.items[i].iden.begin,
               i < ast.funcs.count - 1 ? ", " : "\n");
    }
    printf("CST: \n");
    ast_print_node(ast.top, 0);
}
*/
void ast_free_node(AST_Node * node) {
    for (size_t i = 0; i < node->count; ++i) {
        ast_free_node(node->items + i);
    }
    da_free(node);
}
/*
void ast_free(AST * ast) {
    ast_free_node(&ast->top);
    da_free(&ast->vars);
    da_free(&ast->funcs);
}
*/


/*size_t ast_symbol_find(symbol) {

  }*/

// main implementations

typedef struct {
    // SymbolList *
    AST_Node * parent;
    Token * begin;
    Token * end; // past end
} AST_Builder_Frame;

int ast_parse_DECL(AST_Builder_Frame * frame);
int ast_parse_FUNC(AST_Builder_Frame * frame);
int ast_parse_BLCK(AST_Builder_Frame * frame);

int ast_parse_stmt(AST_Builder_Frame * frame);
int ast_parse_IFEL(AST_Builder_Frame * frame);
int ast_parse_WHIL(AST_Builder_Frame * frame);
int ast_parse_RETN(AST_Builder_Frame * frame);
int ast_parse_EXPS(AST_Builder_Frame * frame);

int ast_parse_EXPR(AST_Builder_Frame * frame);

int ast_parse_atom(AST_Builder_Frame * frame);
int ast_parse_VARR(AST_Builder_Frame * frame);
int ast_parse_CALL(AST_Builder_Frame * frame);
int ast_parse_INTG(AST_Builder_Frame * frame);

int ast_parse_UPOP(AST_Builder_Frame * frame);
int ast_parse_BIOP(AST_Builder_Frame * frame);
int ast_parse_SIGN(AST_Builder_Frame * frame);
int ast_parse_DECM(AST_Builder_Frame * frame);
int ast_parse_IDEN(AST_Builder_Frame * frame);

int ast_parse_exact(AST_Builder_Frame * frame, const char * expect);
int ast_parse_exact_no_discard(AST_Builder_Frame * frame, const char * expect);


#define error(e) { ast_builder_errmsg = e; return 1; }
// free node version
#define error_free(e) { ast_builder_errmsg = e; ast_free_node(&node); return 1; }
const char * errmsg_eof = "Unexpected EOF";


int ast_build(AST_Node * top, Tokenizer * tok) {
    *top = (AST_Node) {'TOP'};

    AST_Builder_Frame frame = {
        .parent = top,
        .begin  = tok->items,
        .end    = tok->items + tok->count,
    };

    // (DECL|FUNC)*
    while (frame.begin < frame.end) {
        if (ast_parse_DECL(&frame) &&
            ast_parse_FUNC(&frame)) error("Invalid top level code");
    }
    //ast_parse_EXPR(&frame);
    return 0;
}

#define tokstrcmp(tok, str) strncmp(str, (tok)->begin, (tok)->len)


#define check_frame_empty()                                             \
    if (frame->begin >= frame->end) { ast_builder_errmsg = errmsg_eof; return 1; }

// provides `node`, `subframe`
#define parse_init(type_with_apos, e)                                   \
    const char * errmsg = (e);                                          \
    if (frame->begin >= frame->end) { ast_builder_errmsg = errmsg_eof; return 1; } \
    AST_Node node = {type_with_apos};                                   \
    AST_Builder_Frame subframe = {                                      \
        .parent = &node,                                                \
        .begin = frame->begin,                                          \
        .end = frame->end,                                              \
    }

#define parse_fin()                             \
    da_append(frame->parent, node);             \
    frame->begin = subframe.begin;              \
    return 0

/*
#define required(type)
#define required_exact(tok)
#define optional(type)
#define optional_group()
#define repeat_group()
*/


// top level objects
int ast_parse_DECL(AST_Builder_Frame * frame) {
    parse_init('DECL', "Invalid variable declaration syntax");
    if (ast_parse_exact(&subframe, "int")) error_free(errmsg);
    if (ast_parse_IDEN(&subframe)) error_free(errmsg);
    // if (ast_symbol_find()
    while (!ast_parse_exact(&subframe, "[")) { // group repeat 0+
        if (ast_parse_DECM(&subframe)) error_free(errmsg);
        if (ast_parse_exact(&subframe, "]")) error_free(errmsg);
    }
    if (ast_parse_exact(&subframe, ";")) error_free(errmsg);
    parse_fin();
}
int ast_parse_FUNC(AST_Builder_Frame * frame) {
    parse_init('FUNC', "Invalid function definition");
    if (ast_parse_exact(&subframe, "int")) error_free(errmsg);
    if (ast_parse_IDEN(&subframe)) error_free(errmsg);
    
    if (ast_parse_exact(&subframe, "(")) error_free(errmsg);
    // optional group
    if (!ast_parse_exact(&subframe, "int") &&
            ast_parse_IDEN(&subframe)) error_free(errmsg);
    while (!ast_parse_exact(&subframe, ",")) {
        if (ast_parse_exact(&subframe, "int")) error_free(errmsg);
        if (ast_parse_IDEN(&subframe)) error_free(errmsg);
    }
    if (ast_parse_exact(&subframe, ")")) error_free(errmsg);

    if (ast_parse_BLCK(&subframe)) error_free(errmsg);
    
    parse_fin();
}
int ast_parse_BLCK(AST_Builder_Frame * frame) {
    parse_init('BLCK', "Invalid block syntax");
    if (ast_parse_exact(&subframe, "{")) error_free(errmsg);
    while (!ast_parse_stmt(&subframe)); // group repeat 0+
    if (ast_parse_exact(&subframe, "}")) error_free(errmsg);
    parse_fin();
}

// stmt
int ast_parse_stmt(AST_Builder_Frame * frame) {
    // alternative structure
    if (ast_parse_DECL(frame) &&
        ast_parse_EXPS(frame) &&
        ast_parse_RETN(frame) &&
        ast_parse_IFEL(frame) &&
        ast_parse_WHIL(frame)) return 1;
    return 0;
}
int ast_parse_IFEL(AST_Builder_Frame * frame) {
    parse_init('IFEL', "Invalid if-else statement");
    if (ast_parse_exact(&subframe, "if")) error_free(errmsg);
    if (ast_parse_exact(&subframe, "(")) error_free(errmsg);
    if (ast_parse_EXPR(&subframe)) error_free(errmsg);
    if (ast_parse_exact(&subframe, ")")) error_free(errmsg);
    // alternative structure
    if (ast_parse_BLCK(&subframe) && ast_parse_stmt(&subframe)) error_free(errmsg);
    // optional group
    if (!ast_parse_exact(&subframe, "else") &&
            ast_parse_BLCK(&subframe) && ast_parse_stmt(&subframe)) error_free(errmsg);
    parse_fin();
}
int ast_parse_WHIL(AST_Builder_Frame * frame) {
    parse_init('WHIL', "Invalid while loop");
    if (ast_parse_exact(&subframe, "while")) error_free(errmsg);
    if (ast_parse_exact(&subframe, "(")) error_free(errmsg);
    if (ast_parse_EXPR(&subframe)) error_free(errmsg);
    if (ast_parse_exact(&subframe, ")")) error_free(errmsg);
    // alternative structure
    if (ast_parse_BLCK(&subframe) && ast_parse_stmt(&subframe)) error_free(errmsg);
    parse_fin();
}
int ast_parse_RETN(AST_Builder_Frame * frame) {
    parse_init('RETN', "Invalid return statement");
    if (ast_parse_exact(&subframe, "return")) error_free(errmsg);
    if (ast_parse_EXPR(&subframe)) error_free(errmsg);
    if (ast_parse_exact(&subframe, ";")) error_free(errmsg);
    parse_fin();
}
int ast_parse_EXPS(AST_Builder_Frame * frame) {
    parse_init('EXPS', "Invalid return statement");
    if (ast_parse_EXPR(&subframe)) error_free(errmsg);
    if (ast_parse_exact(&subframe, ";")) error_free(errmsg);
    parse_fin();
}

// tools for parsing EXPR
int is_open_bracket(Token * tok) {
    return tok->begin[0] == '('
        || tok->begin[0] == '['
        || tok->begin[0] == '{';
}
int is_closed_bracket(Token * tok) {
    return tok->begin[0] == ')'
        || tok->begin[0] == ']'
        || tok->begin[0] == '}';
}
int is_brackets_paired(Token * l, Token * r) {
    return l->begin[0] == '(' && r->begin[0] == ')'
        || l->begin[0] == '[' && r->begin[0] == ']'
        || l->begin[0] == '{' && r->begin[0] == '}';
}

Token * seek_expr_end(AST_Builder_Frame * frame) { // return NULL on fail
    // @algo determining the end of expr
    // - at unmatched bracket
    // - at `;`
    // - at end of frame
    // and that's it!!!!
    
    struct {
        Token * items;
        size_t count; // cursor
        size_t capacity;
    } bracket_stack = {};
    
    Token * cursor = frame->begin;
    // condition 3 in `for` header
    for (; cursor < frame->end; cursor += 1) {
        if (tokstrcmp(cursor, ";") == 0) break; // condition 2
        if (is_open_bracket(cursor)) da_append(&bracket_stack, *cursor); // push
        if (is_closed_bracket(cursor)) {
            if (bracket_stack.count == 0) break; // condition 1
            if (is_brackets_paired(&bracket_stack.items[bracket_stack.count-1],
                                   cursor)) bracket_stack.count -= 1; // pop
            else break; // brackets mismatch
        }
    }
    if (bracket_stack.count > 0) return NULL;
    da_free(&bracket_stack);

    return cursor;
}
const char ops[][4][3] = {
    {"!"}, // 1
    {"*", "/", "%"}, // 2
    {"+", "-"}, // ...
    {"<=", ">=", "<", ">"},
    {"==", "!="},
    {"^"},
    {"&&"},
    {"||"},
    {"="},
    {"<<", ">>"},
    {"("},
};
int op_prec(Token * tok) { // token precedence, -1 for not in list
    size_t maxprec = sizeof(ops) / sizeof(ops[0]);
    size_t res = maxprec - 1;
    while (res >= 0) {
        for (size_t i = 0; i < 4; ++i) {
            if (ops[res][i][0] == '\0') continue;
            if (tok->len != strlen(ops[res][i])) continue;
            if (tokstrcmp(tok, ops[res][i]) == 0) return res+1;
        }
        res -= 1;
    }
    return res;
}
int build_op(AST_Node * parent, AST_Node operator) {
    if (tokstrcmp(operator.token, "!") == 0) {
        if (parent->count < 1) return 1;
        da_append(&operator, parent->items[parent->count - 1]);
        parent->count -= 1;
        da_append(parent, operator);
        return 0;
    }
    // binary op
    if (parent->count < 2) return 1;
    da_append(&operator, parent->items[parent->count - 2]);
    da_append(&operator, parent->items[parent->count - 1]);
    parent->count -= 2;
    da_append(parent, operator);
    return 0;
}
// a unique type: there is need to process a list of nodes at once
int ast_parse_EXPR(AST_Builder_Frame * frame) {
    parse_init('EXPR', "Invalid expression");
    subframe.end = seek_expr_end(frame); // modifying `end` for the first time!!!
    if (!subframe.end) error_free("Invalid expression, brackets mismatch");

    // parse a list of atomics and ops
    AST_Node node_list = {'TEMP'};
    subframe.parent = &node_list; // parse into this temp list
    
    // @algo: what can follow what?
    // abstract out six classes: (start), 1, +, !, (, )
    // (start) followed by 1 !(  -- same as +
    // 0.    1 followed by  +  )
    // 1.    + followed by 1 !(
    // 3.    ! followed by 1 !(
    // 3.    ( followed by 1 !(  -- disallowing ) here
    // 2.    ) followed by  +  )
    // note that ! and ( are exactly the same
    // this table is necessary for building a syntactically correct AST
    // so basically there are just operators and oprands
    int prev_elem_class = 1;
    while (subframe.begin < subframe.end) { // optional repeat
        if (prev_elem_class % 2 == 1 && !ast_parse_atom(&subframe)) { prev_elem_class = 0; continue; }
        if (prev_elem_class % 2 == 0 && !ast_parse_BIOP(&subframe)) { prev_elem_class = 1; continue; }
        if (prev_elem_class % 2 == 1 &&
            (!ast_parse_UPOP(&subframe) || !ast_parse_exact_no_discard(&subframe, "("))) { prev_elem_class = 3; continue; }
        if (prev_elem_class % 2 == 0 && !ast_parse_exact_no_discard(&subframe, ")")) { prev_elem_class = 2; continue; }

        ast_free_node(&node_list);
        error_free(errmsg);
    }
    
    // @algo: build AST from infix expression
    // if operand: add to parent node
    // if op: pop till a lower precedence op, then push
    //   note (extendability): lower / lowerOrEqual depends on the ASSOCIATIVITY of the binop
    // if (: push
    // if ): pop till (, or error
    
    // pop: check available operands, pick 1~2 make a tree

    /* AST_Node * frame_orig_begin = frame->parent->items; // prevent oob access */
    AST_Node op_stack = {'TEMP'};
#define error_expr_cleanup(e)                   \
    do {                                        \
        free(op_stack.items);                   \
        ast_free_node(&node_list);                    \
        ast_builder_errmsg = e;                 \
        free(node.items);                       \
        return 1;                               \
    } while (0)

    for (AST_Node * elem = node_list.items; elem - node_list.items < node_list.count; ++elem) {
        if (elem->type == 'INTG' ||
            elem->type == 'VARR' ||
            elem->type == 'CALL') { // operand
            da_append(&node, *elem);
            continue;
        }
        if (elem->type == 'BIOP' ||
            elem->type == 'UPOP') {
            int thisprec = op_prec(elem->token);
            while (op_stack.count > 0 &&
                   thisprec >= op_prec(op_stack.items[op_stack.count - 1].token)) {
                // pop
                if (build_op(&node, op_stack.items[op_stack.count - 1])) error_expr_cleanup(errmsg);
                op_stack.count -= 1;
            }
            // ok if count == 0
            da_append(&op_stack, *elem);
            continue;
        }
        if (elem->type == 'EXCT' && tokstrcmp(elem->token, "(") == 0) {
            da_append(&op_stack, *elem);
            continue;
        }
        if (elem->type == 'EXCT' && tokstrcmp(elem->token, ")") == 0) {
            while (op_stack.count > 0 &&
                   tokstrcmp(op_stack.items[op_stack.count - 1].token, "(") != 0) {
                // pop
                if (build_op(&node, op_stack.items[op_stack.count - 1])) error_expr_cleanup(errmsg);
                op_stack.count -= 1;
            }
            // this should never happen! dealt within `seek_expr_end`
            if (op_stack.count == 0) error_expr_cleanup(errmsg);
            // delete the open parenthesis
            op_stack.count -= 1;
            continue;
        }
        // @assert unreachable
    }
    while (op_stack.count > 0 &&
           !build_op(&node, op_stack.items[op_stack.count - 1])) op_stack.count -= 1;
    if (op_stack.count > 0) error_expr_cleanup(errmsg);

    // cleanup
    free(op_stack.items); // only delete the container
    free(node_list.items); // this can contain info that overlaps with main AST
    if (node.count > 0) {
        parse_fin();
    } else {
        frame->begin = subframe.begin;
        return 0;
    }
} // EXPR

// atomic expressions
int ast_parse_atom(AST_Builder_Frame * frame) {
    // alternative structure
    if (ast_parse_INTG(frame) &&
        ast_parse_CALL(frame) &&
        ast_parse_VARR(frame)) return 1;
    return 0;
}
int ast_parse_VARR(AST_Builder_Frame * frame) {
    parse_init('VARR', "Invalid array access syntax");
    if (ast_parse_IDEN(&subframe)) error_free(errmsg);
    while (!ast_parse_exact(&subframe, "[")) { // group repeat 0+
        if (ast_parse_EXPR(&subframe)) error_free(errmsg);
        if (ast_parse_exact(&subframe, "]")) error_free(errmsg);
    }
    parse_fin();
}
int ast_parse_CALL(AST_Builder_Frame * frame) {
    parse_init('CALL', "Invalid function call syntax");
    if (ast_parse_IDEN(&subframe)) error_free(errmsg);
    if (ast_parse_exact(&subframe, "(")) error_free(errmsg);
    ast_parse_EXPR(&subframe); // optional
    while (!ast_parse_exact(&subframe, ",")) { // group repeat 0+
        if (ast_parse_EXPR(&subframe)) error_free(errmsg);
    }
    if (ast_parse_exact(&subframe, ")")) error_free(errmsg);
    parse_fin();
}
int ast_parse_INTG(AST_Builder_Frame * frame) {
    parse_init('INTG', "Invalid integer literal");
    ast_parse_SIGN(&subframe); // optional sign
    if (ast_parse_DECM(&subframe)) error_free("Invalid integer literal");
    parse_fin();
}

// directly parse token
int ast_parse_UPOP(AST_Builder_Frame * frame) { // unary prefix op
    check_frame_empty();
    if (tokstrcmp(frame->begin, "!") == 0) {
        da_append(frame->parent, ((AST_Node) {'UPOP', frame->begin}));
        frame->begin += 1;
        return 0;
    }
    return 1;
}
int ast_parse_BIOP(AST_Builder_Frame * frame) {
    check_frame_empty();
    const char binops[][3] = {
        "*", "/", "%",
        "+", "-",
        "<=", ">=", "<", ">",
        "==", "!=",
        "^",
        "&&",
        "||",
        "=", "<<", ">>"
    };
    const size_t n_binops = sizeof(binops) / sizeof(binops[0]);
    for (size_t i = 0; i < n_binops; ++i) {
        if (tokstrcmp(frame->begin, binops[i]) == 0) {
            da_append(frame->parent, ((AST_Node) {'BIOP', frame->begin}));
            frame->begin += 1;
            return 0;
        }
    }
    return 1;
}
int ast_parse_SIGN(AST_Builder_Frame * frame) {
    check_frame_empty();
    if (tokstrcmp(frame->begin, "+") == 0 || tokstrcmp(frame->begin, "-") == 0) {
        da_append(frame->parent, ((AST_Node) {'SIGN', frame->begin}));
        frame->begin += 1;
        return 0;
    }
    return 1;
}
int ast_parse_DECM(AST_Builder_Frame * frame) {
    check_frame_empty();
    if (!isnumber(frame->begin->begin[0])) return 1;
    for (size_t i = 1; i < frame->begin->len; ++i) {
        if (!isnumber(frame->begin->begin[i])) error("Invalid integer literal");
    }
    da_append(frame->parent, ((AST_Node) {'DECM', frame->begin}));
    frame->begin += 1;
    return 0;
}
int isalnum_(char c) { return c == '_' || isalnum((unsigned char)c); }
int isalpha_(char c) { return c == '_' || isalpha((unsigned char)c); }
int ast_parse_IDEN(AST_Builder_Frame * frame) { // TBD: filter keywords
    check_frame_empty();
    if (!isalpha_(frame->begin->begin[0])) error("Invalid identifier");
    for (size_t i = 1; i < frame->begin->len; ++i) {
        if (!isalnum_(frame->begin->begin[i])) error("Invalid identifier");
    }
    da_append(frame->parent, ((AST_Node) {'IDEN', frame->begin}));
    frame->begin += 1;
    return 0;
}
int ast_parse_exact(AST_Builder_Frame * frame, const char * expect) {
    check_frame_empty();
    if (tokstrcmp(frame->begin, expect) != 0) error("Exact match failed");
    frame->begin += 1;
    return 0;
}
int ast_parse_exact_no_discard(AST_Builder_Frame * frame, const char * expect) {
    check_frame_empty();
    if (tokstrcmp(frame->begin, expect) != 0) error("Exact match failed");
    da_append(frame->parent, ((AST_Node) {'EXCT', frame->begin}));
    frame->begin += 1;
    return 0;
}

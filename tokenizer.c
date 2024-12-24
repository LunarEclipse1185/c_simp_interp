#include <stddef.h> // size_t
#include <ctype.h> // isalnum
#include <string.h> // strlen, cmp
#include <stdio.h> // file

#include "tokenizer.h"
#include "dynarray.h"

#define check_pointer(p, err)                   \
    do {                                        \
        if (!p) {                               \
            printf("ERROR: "err);               \
            return 1;                           \
        }                                       \
    } while (0)


// tokens:
// iden & number: [a-zA-Z0-9_]+
// string literal: "[^"]*"  -- nonexistent
// op: +-*/%
// ! < > <= >= == != && || ^
// << >>
// (){}[]
// ;

int Tokenizer_read_file(Tokenizer * t, const char * path) { // return 1 on fail
    FILE * fp = fopen(path, "r");
    check_pointer(fp, "Tokenizer failed to open file");
    
    fseek(fp, 0, SEEK_END);
    size_t filelen = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    t->buffer = (char *)malloc(filelen + 1);
    check_pointer(t->buffer, "Tokenizer malloc failed");
    fread(t->buffer, 1, filelen, fp);
    t->buffer[filelen] = '\0';
    
    fclose(fp);
    return 0;
}

void Tokenizer_free(Tokenizer * t) {
    free(t->buffer);
    free(t->items);
    t->count = t->capacity = 0;
    // error record intact
}

void Tokenizer_print_around(Tokenizer * t, size_t index, size_t halflen) {
    size_t start = (index - halflen >= 0) ? index - halflen : 0;
    size_t len = (strlen(t->buffer) - index < halflen) ? strlen(t->buffer) - index + halflen : (halflen * 2);
    printf("...%.*s...\n", (int)len, t->buffer + start);
}

size_t parse_token(const char * view, const char ** p_errmsg) {
    const char ctoks[][3] = { // fixed tokens enumerated
        "<=", ">=", "==", "!=", "&&", "||", "<<", ">>",
        "=", "+", "-", "*", "/", "%", "!", "^", "<", ">", ",", ";",
        "(", ")", "[", "]", "{", "}",
    };
    const size_t n_ctoks = sizeof(ctoks) / sizeof(ctoks[0]);

    if (!view || view[0] == '\0') return 0;

    // alphanumeral_
    if (isalnum((unsigned char)view[0]) || view[0] == '_') {
        size_t len = 1;
        while (isalnum((unsigned char)view[len]) || view[len] == '_') {
            len += 1;
        }
        // ok if encounters \0
        return len;
    }
    /*
    // string literal
    if (view[0] == '"') {
        size_t len = 1;
        while (view[len] != '"' && view[len] != '\0') {
            len += 1;
        }
        if (view[len] == '\0') {
            *p_errmsg = "Unclosed string literal";
        }
        return len;
    }*/
    // fixed tokens
    for (size_t i = 0; i < n_ctoks; ++i) {
        size_t toklen = strlen(ctoks[i]);
        if (strncmp(view, ctoks[i], toklen) == 0) {
            return toklen;
        }
    }
    // undefined token
    *p_errmsg = "Unrecognized token";
    return 0;
}

const char * parse_trim_whitespace(const char * view) {
    while (isspace(*view)) view += 1; // safe for '\0'
    return view;
}

int Tokenizer_tokenize(Tokenizer * t) { // return 1 if tokenizer fails
    if (!t->buffer) {
        t->errmsg = "No buffer provided";
        t->errind = 0;
        return 1;
    }
    
    const char * view = t->buffer;
    view = parse_trim_whitespace(view);
    while (view[0] != '\0') {
        size_t len = parse_token(view, &t->errmsg);
        if (t->errmsg) {
            t->errind = view - t->buffer;
            return 1;
        }
        da_append(t, ((Token) {view, len}));
        view += len; // @assert still in bound
        view = parse_trim_whitespace(view);
    }
    return 0;
}

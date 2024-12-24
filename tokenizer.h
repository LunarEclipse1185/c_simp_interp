#ifndef TOKENIZER_H_
#define TOKENIZER_H_

#include <stddef.h> // size_t

typedef struct {
    const char * begin; // weak ref
    size_t len;
} Token;

typedef struct {
    char * buffer;
    
    const char * errmsg; // human readable message, not heap alloc'ed
    size_t errind; // at which char the error occurs

    // token list
    Token * items;
    size_t count;
    size_t capacity;
} Tokenizer;

int Tokenizer_read_file(Tokenizer * t, const char * path);
void Tokenizer_free(Tokenizer * t);
void Tokenizer_print_around(Tokenizer * t, size_t index, size_t halflen);
int Tokenizer_tokenize(Tokenizer * t); // return 1 if tokenizer fails

#endif // TOKENIZER_H_

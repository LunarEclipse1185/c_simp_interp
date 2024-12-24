// a "c with cin/cout" interpreter, with many simplifications

#include <stdio.h>
#include <stddef.h> // size_t
#include <stdlib.h> // memory

#include "dynarray.h"
#include "tokenizer.h"
#include "ast_builder.h"
#include "cvm.h"

void print_tokens(Tokenizer * t) {
    printf("Parsed %zu tokens: ", t->count);
    for (size_t i = 0; i < t->count; ++i) {
        printf("\"%.*s\"%s", (int)t->items[i].len, t->items[i].begin,
               i < t->count - 1 ? ", " : "\n");
    }
}

int main(int argc, char ** argv) {
    int status = 0;
    
    if (argc == 1) {
        printf("ERROR: No input file provided.\n");
        printf("Usage: %s <c-code-file> [<input-file> [<output file>]]\n", argv[0]);
        printf("       If no in/out file is provided, stdin/out "
               "will be used, respectively.\n");
        return 1;
    }
    
    // tokenize
    Tokenizer tok = {};
    status = Tokenizer_read_file(&tok, argv[1]);
    if (status) return status;
    
    status = Tokenizer_tokenize(&tok);
    if (status) {
        printf("Tokenizer error: %s\nAt: ", tok.errmsg);
        Tokenizer_print_around(&tok, tok.errind, 5);
        return status;
    }
    //print_tokens(&tok);
    
    // ast
    AST_Node ast = {};
    status = ast_build(&ast, &tok);
    /*
    printf("Generated AST:\n");
    ast_print_node(ast, 0);
    */
    if (status) {
        printf("AST Builder Error: %s\n", ast_builder_errmsg);
        // currently, all error messages will be overwritten by "Invalid top level code"
        // error system TBD
        return status;
    }
    
    
    // vm

    // open files
    FILE * is = stdin;
    FILE * os = stdout;
    if (argc >= 3) {
        is = fopen(argv[2], "r");
        if (!is) {
            printf("ERROR: Open input file %s failed.\n", argv[2]);
            return 1;
        }
    }
    if (argc >= 4) {
        os = fopen(argv[3], "w");
        if (!os) {
            printf("ERROR: Open output file %s failed.\n", argv[3]);
            return 1;
        }
    }
    
    int ret_val = -1;
    status = cvm_run(&ret_val, &ast, is, os);
    if (status) {
        printf("CVM exited abnormally. Syntax error in source file.\n");
        return status;
    }
    printf("CVM exited successfully with return value %d\n", ret_val);
    
    // cleanup
    Tokenizer_free(&tok);
    ast_free_node(&ast);
    if (is && is != stdin) fclose(is);
    if (os && os != stdout) fclose(os);
    

    
    return status; // should be 0
}

#include <stdio.h>
#include <stdlib.h>
#include "linenoise.h"
#include "mpc.h"

int main() {
    // parsers
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Program = mpc_new("program");

    // defining the grammar
    mpca_lang(MPCA_LANG_DEFAULT,
    "                                                       \
        number   : /-?[0-9]+/ ;                             \
        operator : '+' | '-' | '*' | '/' ;                  \
        expr     : <number> | '(' <operator> <expr>+ ')' ;  \
        program  : /^/ '(' <operator> <expr>+ ')' /$/ ;     \
    ",
    Number, Operator, Expr, Program);

    // intro message
    puts("Lispy version 0.0.1");
    puts("Press Ctrl+C to exit\n");

    // load history at startup
    linenoiseHistoryLoad("history.txt");

    char* line;
    mpc_result_t r;

    // never ending loop
    while ((line = linenoise("lispy> ")) != NULL) {

        // valid string?
        if (line[0] != '\0' && line[0] != '/') {
            // save to history
            linenoiseHistoryAdd(line);
            linenoiseHistorySave("history.txt");

            // parse input
            if (mpc_parse("<stdin>", line, Program, &r)) {
                // print AST
                mpc_ast_print(r.output);
                mpc_ast_delete(r.output);
            } else {
                // print the error
                mpc_err_print(r.error);
                mpc_err_delete(r.error);
            }
        }

        // the typed string is returned as a malloc()
        // hence needs to be freed manually
        free(line);
    }

    // cleanup our parsers
    mpc_cleanup(4, Number, Operator, Expr, Program);

    return 0;
}

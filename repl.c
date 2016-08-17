#include <stdio.h>
#include <stdlib.h>
#include "linenoise.h"
#include "mpc.h"

long power(long x, long y) {
    if (y == 0) { return 1; }
    long r = x;
    for (int i = 2; i <= y; i++) {
        r = r * x;
    }
    return r;
}

long eval_op(long x, char* op, long y) {
    if (strcmp(op, "+") == 0) { return x + y; }
    if (strcmp(op, "-") == 0) { return x - y; }
    if (strcmp(op, "*") == 0) { return x * y; }
    if (strcmp(op, "/") == 0) { return x / y; }
    if (strcmp(op, "%") == 0) { return x % y; }
    if (strcmp(op, "^") == 0) { return power(x, y); }
    return 0;
}

long eval(mpc_ast_t* tree) {
    // if not an expression node
    if (!strstr(tree->tag, "expr")) {
        return eval(tree->children[1]);
    }

    // if the tag has the string `number`
    if (strstr(tree->tag, "number")) {
        return atoi(tree->contents);
    }

    // since '(' is first, operator is the second child
    char *op = tree->children[1]->contents;

    // eval the first operand
    long x = eval(tree->children[2]);

    // check if - is a unary operator
    if (tree->children_num == 4 && (strcmp(op, "-") == 0)) {
        return -1 * x;
    }

    // and use that too evaluate the other children
    int i = 3;
    while (strstr(tree->children[i]->tag, "expr")) {
        x = eval_op(x, op, eval(tree->children[i]));
        i++;
    }
    return x;
}

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
        operator : '+' | '-' | '*' | '/' | '%' | '^' ;       \
        expr     : <number> | '(' <operator> <expr>+ ')' ;  \
        program  : /^/ <expr> /$/ ;                         \
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
                printf("%ld\n", eval(r.output));
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

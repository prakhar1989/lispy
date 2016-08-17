#include <stdio.h>
#include <stdlib.h>
#include "linenoise.h"
#include "mpc.h"

// lisp value
typedef struct {
    int type;
    long num;
    int err;
} lval;

// lisp val types
enum { LVAL_NUM, LVAL_ERR };

// lisp val error types
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

// create new number type lisp value
lval lval_num(long x) {
    lval v;
    v.type = LVAL_NUM;
    v.num = x;
    return v;
}

// create new number type lisp value
lval lval_err(int x) {
    lval v;
    v.type = LVAL_ERR;
    v.err = x;
    return v;
}

// pretty printing lisp values
void lval_print(lval v) {
    switch (v.type) {
        case LVAL_NUM: printf("%li", v.num); break;
        case LVAL_ERR:
            switch (v.err) {
                case LERR_DIV_ZERO: printf("Error: Division by zero!"); break;
                case LERR_BAD_OP: printf("Error: Invalid Operator!"); break;
                case LERR_BAD_NUM: printf("Error: Invalid Number!"); break;
            }
        break;
    }
}

// wrapper for printing with a newline
void lval_println(lval v) { lval_print(v); putchar('\n'); }

long power(long x, long y) {
    if (y == 0) { return 1; }
    long r = x;
    for (int i = 2; i <= y; i++) {
        r = r * x;
    }
    return r;
}

lval eval_op(lval x, char* op, lval y) {
    if (x.type == LVAL_ERR) { return x; }
    if (y.type == LVAL_ERR) { return y; }

    if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
    if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
    if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
    if (strcmp(op, "/") == 0) {
        if (y.num == 0) {
            return lval_err(LERR_DIV_ZERO);
        }
        return lval_num(x.num / y.num);
    }
    if (strcmp(op, "%") == 0) { return lval_num(x.num % y.num); }
    if (strcmp(op, "^") == 0) { return lval_num(power(x.num, y.num)); }
    return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* tree) {
    // if not an expression node
    if (!strstr(tree->tag, "expr")) {
        return eval(tree->children[1]);
    }

    // if the tag has the string `number`
    if (strstr(tree->tag, "number")) {
        // checking if long conversion works or not
        errno = 0;
        long x = strtol(tree->contents, NULL, 10);
        return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
    }

    // since '(' is first, operator is the second child
    char *op = tree->children[1]->contents;

    // eval the first operand
    lval x = eval(tree->children[2]);

    // check if - is a unary operator
    if (tree->children_num == 4 && (strcmp(op, "-") == 0)) {
        return lval_num(-1 * x.num);
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
                lval_println(eval(r.output));
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

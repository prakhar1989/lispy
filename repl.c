#include <stdio.h>
#include <stdlib.h>
#include "linenoise.h"
#include "mpc.h"

// lisp value
typedef struct lval {
    int type;
    long num;

    // error and symbol types have some string data
    char* err;
    char* sym;

    // S-Expressions are variable length lists of other values
    // so we keep track of that number with count
    int count;
    // and store the actual value (each as a cell)
    struct lval** cell;
} lval;

// lisp val types
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR };

// construct a pointer to a new number type lisp value
lval* lval_num(long x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

// construct a pointer to a err type lisp value
lval* lval_err(char* m) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->err = malloc(strlen(m) + 1);
    strcpy(v->err, m);
    return v;
}

// construct a pointer to a sym type lisp value
lval* lval_sym(char* s) {
    lval* v = malloc(sizeof(s));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;
}

// construct a pointer to a sexpr type lisp value
lval* lval_sexpr() {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

// adds a lval x into v's cell
lval* lval_add(lval* v, lval* x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count-1] = x;
    return v;
}

// generates a num type lval from a AST node
lval* lval_read_num(mpc_ast_t* node) {
    // checking if long conversion works or not
    errno = 0;
    long x = strtol(node->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}

// generates a lval from a AST node
lval* lval_read(mpc_ast_t* node) {
   // symbols and numbers are handled simply
   if (strstr(node->tag, "number")) { return lval_read_num(node); }
   if (strstr(node->tag, "symbol")) { return lval_sym(node->contents); }

   // if root(>) or sexpr then create empty list
   lval* x = NULL;
   if (strcmp(node->tag, ">") == 0 || strstr(node->tag, "sexpr")) {
       x = lval_sexpr();
   }

   for (int i = 0; i < node->children_num; i++) {
       if (strcmp(node->children[i]->contents, "(") == 0) { continue; }
       if (strcmp(node->children[i]->contents, ")") == 0) { continue; }
       if (strcmp(node->children[i]->contents, "}") == 0) { continue; }
       if (strcmp(node->children[i]->contents, "{") == 0) { continue; }
       if (strcmp(node->children[i]->tag, "regex") == 0) { continue; }
       x = lval_add(x, lval_read(node->children[i]));
   }
   return x;
}

// free the memory associated with a lval correctly
void lval_del(lval* v) {

    // free allocated memory for elements
    switch (v->type) {
        case LVAL_NUM: break;
        case LVAL_ERR: free(v->err); break;
        case LVAL_SYM: free(v->sym); break;
        case LVAL_SEXPR:
            for (int i = 0; i < v->count; i++) {
                lval_del(v->cell[i]);
            }
            free(v->cell);
        break;
    }

    // free memory allocated to the struct itself
    free(v);
}

// forward declration
void lval_print(lval* v);

void lval_expr_print(lval* v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->count; i++) {
        lval_print(v->cell[i]);
        if (i != (v->count-1)) {
            putchar(' ');
        }
    }
    putchar(close);
}

// pretty printing lisp values
void lval_print(lval* v) {
    switch (v->type) {
        case LVAL_NUM: printf("%li", v->num); break;
        case LVAL_ERR: printf("Error: %s", v->err); break;
        case LVAL_SYM: printf("%s", v->sym); break;
        case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
    }
}

// wrapper for printing with a newline
void lval_println(lval* v) { lval_print(v); putchar('\n'); }

long power(long x, long y) {
    if (y == 0) { return 1; }
    long r = x;
    for (int i = 2; i <= y; i++) {
        r = r * x;
    }
    return r;
}

int main() {
    // parsers
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Symbol = mpc_new("symbol");
    mpc_parser_t* Sexpr = mpc_new("sexpr");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Program = mpc_new("program");

    // defining the grammar
    mpca_lang(MPCA_LANG_DEFAULT,
    "                                                       \
        number   : /-?[0-9]+/ ;                             \
        symbol   : '+' | '-' | '*' | '/' | '%' | '^' ;      \
        sexpr    : '(' <expr>* ')';                         \
        expr     : <number> | <symbol> | <sexpr> ;          \
        program  : /^/ <expr>* /$/ ;                        \
    ",
    Number, Symbol, Sexpr, Expr, Program);

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
                mpc_ast_print(r.output);
                lval* x = lval_read(r.output);
                lval_println(x);
                lval_del(x);
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
    mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Program);

    return 0;
}

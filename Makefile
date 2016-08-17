all: lisp

lisp: repl.c
	$(CC) -std=c99 -Os -g -Wall -o lisp.out linenoise.c mpc.c repl.c

clean:
	rm *.out
	rm -rf *.dSYM

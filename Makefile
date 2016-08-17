all: lisp

lisp: repl.c
	$(CC) -std=c99 -Os -g -Wall -o lisp linenoise.c repl.c

clean:
	rm lisp

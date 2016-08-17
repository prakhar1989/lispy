#include <stdio.h>
#include "linenoise.h"

int main() {
    puts("Lispy version 0.0.1");
    puts("Press Ctrl+C to exit\n");

    // never ending loop
    while (1) {
        char* line = linenoise("lispy> ");
        printf("You entered: %s\n", line);
        free(line);
    }

    return 0;
}

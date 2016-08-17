#include <stdio.h>
#include <stdlib.h>
#include "linenoise.h"

int main() {
    puts("Lispy version 0.0.1");
    puts("Press Ctrl+C to exit\n");

    // load history at startup
    linenoiseHistoryLoad("history.txt");

    char* line;

    // never ending loop
    while ((line = linenoise("lispy> ")) != NULL) {

        // valid string?
        if (line[0] != '\0' && line[0] != '/') {
            // save to history
            linenoiseHistoryAdd(line);
            linenoiseHistorySave("history.txt");

            // echo back the response
            printf("You entered: %s\n", line);
        }

        // the typed string is returned as a malloc()
        // hence needs to be freed manually
        free(line);
    }

    return 0;
}

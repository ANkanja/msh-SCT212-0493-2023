/*
 * msh - a minimal Unix shell for teaching.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    char *line = NULL;
    size_t cap = 0;

    for (;;) {
        printf("msh> ");
        fflush(stdout);

        /* getline grows the buffer as needed, so lines can be any length */
        if (getline(&line, &cap, stdin) == -1) {
            /* end-of-file (Ctrl-D) or read error: leave cleanly */
            putchar('\n');
            break;
        }
    }

    free(line);
    return 0;
}

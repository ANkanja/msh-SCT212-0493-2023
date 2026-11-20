/*
 * msh - a minimal Unix shell for teaching.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DELIMS " \t\r\n"

/*
 * Split line in place into words separated by spaces/tabs.
 * Returns a malloc'd, NULL-terminated array of pointers into line,
 * and stores the word count in *argc. Caller frees the array only.
 */
static char **split_line(char *line, int *argc)
{
    size_t cap = 8;
    int n = 0;
    char **argv = malloc(cap * sizeof *argv);
    if (argv == NULL) {
        perror("msh: malloc");
        exit(1);
    }

    for (char *tok = strtok(line, DELIMS); tok != NULL;
         tok = strtok(NULL, DELIMS)) {
        if ((size_t)n + 1 >= cap) {          /* keep room for NULL */
            cap *= 2;
            char **tmp = realloc(argv, cap * sizeof *argv);
            if (tmp == NULL) {
                perror("msh: realloc");
                free(argv);
                exit(1);
            }
            argv = tmp;
        }
        argv[n++] = tok;
    }
    argv[n] = NULL;
    *argc = n;
    return argv;
}

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

        int argc;
        char **argv = split_line(line, &argc);
        if (argc == 0) {                     /* blank line */
            free(argv);
            continue;
        }

        /* temporary: show the parsed words */
        for (int i = 0; i < argc; i++)
            printf("argv[%d] = '%s'\n", i, argv[i]);

        free(argv);
    }

    free(line);
    return 0;
}

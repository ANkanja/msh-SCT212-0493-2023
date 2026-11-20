/*
 * msh - a minimal Unix shell for teaching.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

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

/*
 * Built-in commands run inside the shell process itself.
 * Returns 1 if argv[0] was a built-in (and handled), 0 otherwise.
 * Sets *should_exit when the user typed exit.
 */
static int run_builtin(char **argv, int argc, int *should_exit)
{
    if (strcmp(argv[0], "exit") == 0) {
        *should_exit = 1;
        return 1;
    }

    if (strcmp(argv[0], "cd") == 0) {
        const char *dir = argv[1];
        if (dir == NULL) {
            dir = getenv("HOME");
            if (dir == NULL) {
                fprintf(stderr, "msh: cd: HOME not set\n");
                return 1;
            }
        } else if (argc > 2) {
            fprintf(stderr, "msh: cd: too many arguments\n");
            return 1;
        }
        if (chdir(dir) != 0)
            perror("msh: cd");
        return 1;
    }

    if (strcmp(argv[0], "help") == 0) {
        printf("msh - minimal shell\n"
               "Built-in commands:\n"
               "  cd [dir]   change directory (default: $HOME)\n"
               "  help       show this message\n"
               "  exit       leave the shell\n"
               "Anything else is run as an external program.\n");
        return 1;
    }

    return 0;
}

/*
 * Run an external program: fork a child, replace the child with the
 * program via execvp, and make the parent wait for it with waitpid.
 */
static void run_external(char **argv)
{
    pid_t pid = fork();
    if (pid < 0) {
        perror("msh: fork");
        return;
    }

    if (pid == 0) {
        /* child: becomes the requested program, or exits on failure */
        execvp(argv[0], argv);
        if (errno == ENOENT)
            fprintf(stderr, "msh: %s: command not found\n", argv[0]);
        else
            fprintf(stderr, "msh: %s: %s\n", argv[0], strerror(errno));
        _exit(errno == ENOENT ? 127 : 126);
    }

    /* parent: wait for this specific child to finish */
    int status;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            perror("msh: waitpid");
            return;
        }
    }
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

        int should_exit = 0;
        if (!run_builtin(argv, argc, &should_exit))
            run_external(argv);
        free(argv);
        if (should_exit)
            break;
    }

    free(line);
    return 0;
}

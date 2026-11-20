/*
 * msh - a minimal Unix shell for teaching.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define DELIMS " \t\r\n"

/*
 * SIGINT handler for the shell itself. Ctrl-C sends SIGINT to every
 * process in the terminal's foreground group, so the running child
 * gets it and dies, while the shell just moves to a fresh line.
 * Only async-signal-safe calls (write) are allowed here.
 */
static void on_sigint(int sig)
{
    (void)sig;
    write(STDOUT_FILENO, "\n", 1);
}

static void install_sigint_handler(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;   /* no SA_RESTART: getline returns so we can re-prompt */
    if (sigaction(SIGINT, &sa, NULL) < 0)
        perror("msh: sigaction");
}

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
 * Convert a raw waitpid() status into a shell exit status:
 * normal exit -> the exit code, killed by signal N -> 128 + N.
 */
static int decode_status(int status)
{
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);
    return 1;
}

/*
 * Built-in commands run inside the shell process itself.
 * Returns -1 if argv[0] is not a built-in, otherwise the command's
 * exit status. Sets *should_exit when the user typed exit.
 */
static int run_builtin(char **argv, int argc, int *should_exit)
{
    if (strcmp(argv[0], "exit") == 0) {
        *should_exit = 1;
        return 0;
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
        if (chdir(dir) != 0) {
            perror("msh: cd");
            return 1;
        }
        return 0;
    }

    if (strcmp(argv[0], "help") == 0) {
        printf("msh - minimal shell\n"
               "Built-in commands:\n"
               "  cd [dir]   change directory (default: $HOME)\n"
               "  help       show this message\n"
               "  exit       leave the shell\n"
               "Anything else is run as an external program.\n");
        return 0;
    }

    return -1;
}

/*
 * If the command ends with '&' (as its own word or stuck to the last
 * word, e.g. "sleep 3&"), remove it and return 1. Otherwise return 0.
 */
static int strip_background(char **argv, int *argc)
{
    if (*argc == 0)
        return 0;

    char *last = argv[*argc - 1];
    size_t len = strlen(last);
    if (len == 0 || last[len - 1] != '&')
        return 0;

    if (len == 1) {                      /* "&" was its own word */
        argv[--(*argc)] = NULL;
    } else {                             /* "word&" */
        last[len - 1] = '\0';
    }
    return 1;
}

/*
 * Find "> file" (or ">file") in argv, remove it, and store the file
 * name in *outfile (NULL if there is no redirection).
 * Returns 0 on success, -1 on a syntax error such as a missing name.
 */
static int parse_redirect(char **argv, int *argc, char **outfile)
{
    *outfile = NULL;
    for (int i = 0; i < *argc; i++) {
        if (argv[i][0] != '>')
            continue;

        int used;                        /* how many words to remove */
        if (argv[i][1] != '\0') {        /* ">file" */
            *outfile = argv[i] + 1;
            used = 1;
        } else if (i + 1 < *argc) {      /* "> file" */
            *outfile = argv[i + 1];
            used = 2;
        } else {
            fprintf(stderr, "msh: syntax error: expected file after '>'\n");
            return -1;
        }

        /* shift the remaining words left over the removed ones */
        for (int j = i; j + used <= *argc; j++)
            argv[j] = argv[j + used];
        *argc -= used;
        return 0;
    }
    return 0;
}

/*
 * Reap every background child that has finished, without blocking.
 * WNOHANG makes waitpid return 0 immediately if nobody is done yet,
 * so finished jobs never linger as zombies.
 */
static void reap_background(void)
{
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
        printf("[done] %d (status %d)\n", (int)pid, decode_status(status));
}

/*
 * Run an external program: fork a child, replace the child with the
 * program via execvp, and make the parent wait for it with waitpid.
 * In background mode the parent prints the PID and returns at once.
 * Returns the command's exit status.
 */
static int run_external(char **argv, int background, const char *outfile)
{
    pid_t pid = fork();
    if (pid < 0) {
        perror("msh: fork");
        return 1;
    }

    if (pid == 0) {
        /* child: foreground jobs get default Ctrl-C behaviour (die);
         * background jobs ignore it so Ctrl-C only hits the foreground */
        signal(SIGINT, background ? SIG_IGN : SIG_DFL);

        /* set up redirection here so the shell's own stdout
         * is never touched */
        if (outfile != NULL) {
            int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                fprintf(stderr, "msh: %s: %s\n", outfile, strerror(errno));
                _exit(1);
            }
            if (dup2(fd, STDOUT_FILENO) < 0) {
                perror("msh: dup2");
                _exit(1);
            }
            close(fd);
        }

        /* become the requested program, or exit on failure */
        execvp(argv[0], argv);
        int err = errno;                 /* save before fprintf touches it */
        if (err == ENOENT)
            fprintf(stderr, "msh: %s: command not found\n", argv[0]);
        else
            fprintf(stderr, "msh: %s: %s\n", argv[0], strerror(err));
        _exit(err == ENOENT ? 127 : 126);
    }

    if (background) {
        /* do not wait; reap_background() will collect it later */
        printf("[bg] %d\n", (int)pid);
        return 0;
    }

    /* parent: wait for this specific child to finish */
    int status;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            perror("msh: waitpid");
            return 1;
        }
    }
    return decode_status(status);
}

int main(void)
{
    char *line = NULL;
    size_t cap = 0;
    int last_status = 0;

    install_sigint_handler();

    for (;;) {
        reap_background();               /* collect finished jobs first */
        printf("msh[%d]> ", last_status);
        fflush(stdout);

        /* getline grows the buffer as needed, so lines can be any length */
        errno = 0;
        if (getline(&line, &cap, stdin) == -1) {
            if (errno == EINTR) {
                /* Ctrl-C at the prompt: discard input, prompt again */
                clearerr(stdin);
                last_status = 130;
                continue;
            }
            /* end-of-file (Ctrl-D) or read error: leave cleanly */
            putchar('\n');
            break;
        }

        int argc;
        char **argv = split_line(line, &argc);
        int background = strip_background(argv, &argc);
        char *outfile;
        if (parse_redirect(argv, &argc, &outfile) < 0) {
            last_status = 2;
            free(argv);
            continue;
        }
        if (argc == 0) {                     /* blank line (or lone "&") */
            free(argv);
            continue;
        }

        int should_exit = 0;
        int rc = run_builtin(argv, argc, &should_exit);
        if (rc < 0)
            rc = run_external(argv, background, outfile);
        last_status = rc;
        free(argv);
        if (should_exit)
            break;
    }

    free(line);
    return 0;
}

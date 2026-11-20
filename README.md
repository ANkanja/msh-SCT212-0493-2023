# msh: a minimal Unix shell

**Name:** Austin Ngunjiri
**Student ID:** SCT212-0493/2023
**Course:** ICS 2305 Systems Programming, practical assignment (Question 1)

`msh` is a small command-line shell written in C for teaching how Unix
runs programs. It reads a command, starts it as a separate process with
`fork()` / `execvp()` / `waitpid()`, and reports the result.

## Build and run

Needs Linux (or WSL) with `gcc` and `make`.

```sh
git clone https://github.com/ANKanja/msh-SCT212-0493-2023.git
cd msh-SCT212-0493-2023
make
./msh
```

`make clean` removes the binary.

Example session:

```
msh[0]> echo hello world
hello world
msh[0]> nosuchcommand
msh: nosuchcommand: command not found
msh[127]> sleep 3 &
[bg] 12345
msh[0]>
```

## Features implemented

| Task | Feature | Status |
|------|---------|--------|
| 1 | Prompt, read lines of any length with `getline()`, clean exit on Ctrl-D | Complete |
| 2 | Split line into a NULL-terminated `argv` on spaces/tabs, ignore blank lines | Complete |
| 3 | Run external commands with `fork` / `execvp` / `waitpid`, report errors, unknown command → status 127 | Complete |
| 4 | Built-ins `cd [dir]` (default `$HOME`), `help`, `exit` | Complete |
| 5 | Previous exit status in the prompt (`msh[N]>`), 128 + signal for killed jobs | Complete |
| 6 | Background jobs with trailing `&`, PID printed, finished jobs reaped with `WNOHANG` | Complete |
| 7 | Output redirection `>` (create/truncate, mode 0644) done in the child | Complete |
| 8 | Ctrl-C stops the foreground command but not the shell | Complete |

Test results are in [tests/test_plan.md](tests/test_plan.md).

## Design explanation

### The fork / exec / wait cycle

When I type a command such as `ls -l`, `fork()` creates a child process
that is a copy of the shell. The program knows which process is running
through the return value: 0 means it's the child, while a positive
number means it's the parent.

In the child, `execvp()` replaces the child process with the `ls`
program and executes the command. If it works, the original child code
is replaced, so the code after `execvp()` only runs if something goes
wrong. In that case msh prints an error and the child exits with 127,
which is what `nosuchcommand` shows.

The parent waits for the child process to finish using `waitpid()`.
Once the child finishes, the parent collects its exit status and uses
it to display the number in the next prompt. If the child was killed by
a signal, the status is 128 plus the signal number. For example, Ctrl-C
sends the SIGINT signal, which has a value of 2, giving 130.

### Why cd and exit are built-ins

If `cd` was a separate program, it would only change the directory of
its own process, not the shell's directory. Once it finishes, the shell
would still be in the original folder. That is why `cd` must run inside
the shell itself using `chdir()`.

`exit` would have the same problem, because it would only terminate the
separate child process. The original shell would continue running.

### Avoiding zombie processes

A zombie process is a process that has finished running but whose
parent has not yet collected its exit status. It stays in the process
table until the parent cleans it up.

Foreground commands are collected straight away by `waitpid()`. For
background commands started with `&`, the shell does not wait. Instead,
before every prompt it uses `waitpid()` with the `WNOHANG` option to
check for completed background processes without waiting for them. This
allows it to clean them up while keeping the prompt responsive.

### Reflection

The hardest part for me was understanding how the parent and child
processes work together, especially how `fork()` and `execvp()`
interact. It was also interesting to learn that a process can finish
running but still remain in the process table as a zombie.

If I had more time, I would add pipes and output redirection using
`>>`. I would also improve command parsing to support quotes and add a
`jobs` command for managing background processes.

## Known limitations

- No pipes (`|`), input redirection (`<`), append (`>>`), quoting, or
  variable expansion. Words are split on whitespace only, so
  `echo "a b"` passes the quotes through to `echo`.
- Redirection and `&` apply to external commands only. `help > file`
  still prints to the terminal.
- No job control (`jobs`, `fg`, `bg`). The shell does not use separate
  process groups, so background jobs are protected from Ctrl-C only by
  ignoring SIGINT.
- Finished background jobs are reported only when the next prompt is
  printed, not the moment they finish.
- `exit` does not take a status argument.

## Sources and AI-tool declaration

<!-- TODO: check and complete this list. -->

- Linux man pages: `fork(2)`, `execvp(3)`, `waitpid(2)`, `getline(3)`,
  `strtok(3)`, `sigaction(2)`, `dup2(2)`, `open(2)`, `chdir(2)`, used to
  check arguments, return values and error handling.
- ICS 2305 lecture notes on processes and signals.
- Claude (Anthropic AI assistant), used to help write and structure the
  code step by step, review error handling, and draft this README and
  the test plan. I tested every feature myself and recorded the results
  in `tests/test_plan.md`.

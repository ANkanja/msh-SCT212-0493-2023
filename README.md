# msh: a minimal Unix shell

**Name:** _YOUR NAME_
**Student ID:** _YOUR STUDENT ID_
**Course:** ICS 2305 Systems Programming, practical assignment (Question 1)

`msh` is a small command-line shell written in C for teaching how Unix
runs programs. It reads a command, starts it as a separate process with
`fork()` / `execvp()` / `waitpid()`, and reports the result.

## Build and run

Needs Linux (or WSL) with `gcc` and `make`.

```sh
git clone https://github.com/YOURUSERNAME/msh-YOURSTUDENTID.git
cd msh-YOURSTUDENTID
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

<!-- TODO: rewrite this section in your own words before submitting. -->

The main loop prints the prompt, reads one line with `getline()` and
splits it into words with `strtok()`. The words are stored in a growing
array of `char *` that always ends in `NULL`, because that is the form
`execvp()` expects. Before running anything the shell removes a trailing
`&` (background flag) and a `> file` pair (output file) from the array.

If the first word is not a built-in, the shell calls `fork()`. This makes
an almost exact copy of the shell process. In the child, `fork()` returns
0: the child sets SIGINT back to its default action, opens the output
file and uses `dup2()` to put it on standard output if there was a `>`,
and then calls `execvp()`. `execvp()` searches `PATH` and replaces the
child's program with the requested one. If it returns at all, it failed,
so the child prints an error and exits with 127 (not found) or 126. In
the parent, `fork()` returns the child's PID. The parent calls
`waitpid()` on that PID until the child ends, then turns the raw status
into a number with `WIFEXITED` / `WEXITSTATUS`, or `128 + WTERMSIG` if a
signal killed it. That number is shown in the next prompt.

`cd` and `exit` have to be built-ins because they change the shell
process itself. The current directory belongs to each process. If `cd`
were a separate program, it would change its own directory in a child
process and then exit, and the shell's directory would not change at
all. In the same way, an external `exit` program could only end itself,
not the shell that started it.

A finished child stays a zombie until its parent collects its status
with `wait`. Foreground children are collected straight away by
`waitpid()`. For background jobs the shell does not wait. Instead,
before every prompt it calls `waitpid(-1, &status, WNOHANG)` in a loop.
`WNOHANG` returns at once when no child has finished, so the prompt is
never blocked, and every finished job is reaped and reported with a
`[done]` line.

Ctrl-C sends SIGINT to every process in the terminal's foreground group.
The shell installs a handler that only prints a newline, so it survives.
The foreground child has the default action, so it dies (status 130).
Background children ignore SIGINT.

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

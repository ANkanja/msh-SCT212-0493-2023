# msh test plan

**Test environment:** Cygwin 3.x on Windows 11, gcc 14.4.0, built with
`make` (`-std=c11 -Wall -Wextra`). It also builds with no warnings
using `-O2 -D_FORTIFY_SOURCE=2`.

Commands were fed to `./msh` on standard input. The number in the
prompt after each command (`msh[N]>`) is the exit status recorded.

Ctrl-C was tested by sending SIGINT to msh's whole process group
(`kill -INT -- -<pgid>`). That is what the terminal driver does when
you press Ctrl-C. Ctrl-D was tested by closing msh's input, which makes
`getline()` return end-of-file just as Ctrl-D at a terminal does.

## Required tests

| # | Input | Expected behaviour | Actual result |
|---|-------|--------------------|---------------|
| 1 | (blank line) | New prompt, no error | New `msh[0]>` prompt, no output. **Pass** |
| 2 | `echo hello world` | Prints text; status 0 | Printed `hello world`, prompt `msh[0]>`. **Pass** |
| 3 | `ls /nonexistent` | Error from `ls`; non-zero status | `ls: cannot access '/nonexistent': No such file or directory`, prompt `msh[2]>`. **Pass** |
| 4 | `nosuchcommand` | Error message; status 127 | `msh: nosuchcommand: command not found`, prompt `msh[127]>`. **Pass** |
| 5 | `cd /tmp` then `pwd` | Prints `/tmp` | Printed `/tmp`, status 0. **Pass** |
| 6 | `sleep 3 &` | Prompt returns at once; PID shown | Printed `[bg] 842` and the next prompt appeared at once. `ps` showed the sleep running in the background. **Pass** |
| 7 | `echo hi > f.txt` then `cat f.txt` | File contains `hi` | `cat f.txt` printed `hi`. A file created in `/tmp` with umask 022 had mode `-rw-r--r--` (0644). **Pass** |
| 8 | `sleep 100` then Ctrl-C | Sleep stops; shell survives; status 130 | Sleep stopped at once, a new line was printed and the prompt became `msh[130]>`. The next command `echo still alive` worked. **Pass** |
| 9 | Ctrl-D at the prompt | Shell exits cleanly | msh printed a newline and exited with status 0. No msh process was left running. **Pass** |

## My own tests

| # | Input | Expected behaviour | Actual result |
|---|-------|--------------------|---------------|
| 10 | `cd` (no argument) then `pwd` | Prints the value of `$HOME` | Printed `/cygdrive/c/Users/Kanja`, which matches `$HOME`. **Pass** |
| 11 | `   echo    a  <TAB>  b   ` (extra spaces and a tab) | Prints `a b`; status 0 | Printed `a b`, prompt `msh[0]>`. **Pass** |
| 12 | `sh tests/selfkill.sh` (script runs `kill -TERM $$`) | Child killed by SIGTERM (15); status 128+15 = 143 | Prompt became `msh[143]>`. **Pass** |
| 13 | `sleep 1&` (no space before `&`), wait, press Enter, run `ps` | PID shown at once; `[done]` line before the next prompt; no zombie left | Printed `[bg] 843`. After it finished, Enter printed `[done] 843 (status 0)`, and the following `ps` listed no sleep processes. **Pass** |
| 14 | `echo hi >` | Syntax error message; status 2; shell keeps running | `msh: syntax error: expected file after '>'`, prompt `msh[2]>`. **Pass** |
| 15 | `ls > /proc/nope/x.txt` (directory does not exist) | Error message from the child; status 1; shell keeps running | `msh: /proc/nope/x.txt: No such file or directory`, prompt `msh[1]>`. **Pass** |
| 16 | Ctrl-C at an empty prompt | New prompt on a fresh line; shell keeps running | New line, then `msh[130]>`. The shell kept accepting commands. **Pass** |
| 17 | `sleep 4 &`, then Ctrl-C | Background job is not killed by Ctrl-C | After Ctrl-C, `ps` still listed the sleep (PID 1697), and it later reported `[done] 1697 (status 0)`. **Pass** |
| 18 | `help` | Lists the built-in commands; status 0 | Printed the help text, prompt `msh[0]>`. **Pass** |
| 19 | `valgrind --leak-check=full ./msh`, run `ls`, then Ctrl-D | "All heap blocks were freed -- no leaks are possible" | _Not run yet: valgrind is not available on Cygwin. To be run on Linux/WSL._ |

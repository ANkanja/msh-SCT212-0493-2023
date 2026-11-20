# msh test plan

All tests were run by typing the input at the `msh[N]>` prompt after
building with `make` and starting `./msh`. The number in the prompt
after each command is the exit status that was recorded.

## Required tests

| # | Input | Expected behaviour | Actual result |
|---|-------|--------------------|---------------|
| 1 | (blank line) | New prompt, no error | _to fill in_ |
| 2 | `echo hello world` | Prints text; status 0 | _to fill in_ |
| 3 | `ls /nonexistent` | Error from `ls`; non-zero status | _to fill in_ |
| 4 | `nosuchcommand` | Error message; status 127 | _to fill in_ |
| 5 | `cd /tmp` then `pwd` | Prints `/tmp` | _to fill in_ |
| 6 | `sleep 3 &` | Prompt returns at once; PID shown | _to fill in_ |
| 7 | `echo hi > f.txt` then `cat f.txt` | File contains `hi` | _to fill in_ |
| 8 | `sleep 100` then Ctrl-C | Sleep stops; shell survives; status 130 | _to fill in_ |
| 9 | Ctrl-D at the prompt | Shell exits cleanly | _to fill in_ |

## My own tests

| # | Input | Expected behaviour | Actual result |
|---|-------|--------------------|---------------|
| 10 | `cd` (no argument) then `pwd` | Prints the value of `$HOME` | _to fill in_ |
| 11 | `   echo    a     b   ` (extra spaces/tabs) | Prints `a b`; status 0 | _to fill in_ |
| 12 | `sh -c 'kill -TERM $$'` | Child killed by SIGTERM (15); status 128+15 = 143 | _to fill in_ |
| 13 | `sleep 1&` (no space before `&`), wait 2 s, press Enter | PID shown at once; `[done]` line printed before the next prompt; no `<defunct>` entry in `ps` | _to fill in_ |
| 14 | `echo hi >` | Syntax error message; status 2; shell keeps running | _to fill in_ |
| 15 | `ls > /root/x.txt` (no permission) | `Permission denied` message; status 1; shell keeps running | _to fill in_ |
| 16 | Ctrl-C at an empty prompt | New prompt on a fresh line; shell keeps running | _to fill in_ |
| 17 | `valgrind --leak-check=full ./msh`, run `ls`, then Ctrl-D | "All heap blocks were freed -- no leaks are possible" | _to fill in_ |

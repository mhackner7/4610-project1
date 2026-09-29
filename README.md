# COP 4610 Project 1 shell

This local contribution implements parts 4-6 on top of Chloe Patrick's parts 0-3, and Max Hackner implements parts 7-9.


## Build and run

Use Linux (linprog or WSL Ubuntu), GCC, and GNU Make. The shell uses C99 and POSIX
interfaces, with no external libraries. Native Windows/MinGW is not the target.
From this directory:

```sh
make
./bin/shell
```

`make run` also starts the shell. Leave this build with Ctrl-D on an empty
input line, or use the `exit` command. The `exit`, `cd`, and `jobs` built-ins
are fully implemented as part of Max's part 9. The prompt uses the current username, hostname, and directory.

Examples, entered at the shell prompt:

```text
ls -al
echo $USER
echo ~ ~/Documents
echo hello > result.txt
cat < result.txt
sort < result.txt > sorted.txt
sort > sorted.txt < result.txt
```

Separate `<` and `>` from words with spaces. Commands may use relative or absolute
paths. Missing commands and failed redirects report errors without ending the shell.
Output files are truncated and have mode 0600, including pre-existing files. Input
must be a readable regular file. Input validation precedes output modification.
Redirecting input and output to the same inode is rejected to preserve input contents.

## Tests

Python 3 is needed only for the tests, not the shell. Run as a normal Linux user so
permission tests are meaningful:

```sh
make clean
make
make test
```

Tests create fixtures in temporary Linux directories and remove them afterward.
`make test` also builds a C API test in `obj/`; default `make` produces only `bin/shell`.
The API test deliberately prints diagnostics for expected errors before reporting success.

For memory and undefined-behavior checks on Linux:

```sh
make clean
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
make CFLAGS='-std=c99 -g -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie' \
     LDFLAGS='-fsanitize=address,undefined -no-pie' test
make clean
```

A clean rebuild is necessary when changing compiler flags. Use Linux temporary/home
storage for permission-sensitive fixtures rather than relying on Windows drive modes.
See `docs/TEST_RESULTS.md` for tests actually performed and the linprog checklist.

## File listing

| Files | Purpose |
| --- | --- |
| `src/main.c` | Prompt/read/expand/execute loop and integration location |
| `src/lexer.c`, `include/lexer.h` | Input reading, token-list creation and cleanup |
| `src/shell.c`, `include/shell.h` | Chloe's prompt and expansion functions |
| `src/path_search.c`, `include/path_search.h` | Manual PATH resolution |
| `src/redirection.c`, `include/redirection.h` | Redirect parsing and child descriptor setup |
| `src/execution.c`, `include/execution.h` | fork/execv, error reporting, foreground waits |
| `Makefile` | Builds, dependency tracking, tests and cleanup |
| `.gitignore` | Excludes generated objects, binaries and Python caches |
| `tests/test_shell.py` | 19 black-box regression tests |
| `tests/test_execution.c`, `tests/run_execution_test.py` | Execution status API tests |
| `docs/CHLOE_REVIEW.md` | Baseline review and prerequisite corrections |
| `docs/INTEGRATION.md` | Interfaces and ownership rules for Max |
| `docs/TEST_RESULTS.md` | Actual validation and remaining linprog check |
| `README.md` | Build instructions, assignments, development log and limitations |

`obj/` and `bin/` are created by Make. Run `make clean` before submitting and do not
commit compiled artifacts. The project root is this folder, not the original tar's
`shell/starter/` nesting.

## Development log

### Chloe Patrick

| Date | Work completed |
| --- | --- |
| 2026-09-26 | Parts 0-3 were supplied in `shell_updated.tar`. Chloe established the core foundation of the shell by building the interactive prompt, parsing user input into token lists, and implementing both environment variable and tilde expansion.

### Juan Medina Molina

| Date | Work completed |
| --- | --- |
| 2026-09-28 | Used GPT to review the supplied baseline, correct prerequisite defects, and verify the build in WSL Ubuntu and on linprog6. |

### Max Hackner

| Date | Work completed |
| --- | --- |
| 2026-09-28 | Implemented Parts 7 (Piping), 8 (Background Processing), and 9 (Built-ins: exit, cd, jobs). Integrated pipeline execution seamlessly with Juan's I/O redirection. |

## Meetings

Various meetings throughout various weeks.

## Known limitations and unfinished work

- Quotes, escapes, globs, adjacent operators, append redirects and full Bash grammar
  are not implemented. The required whitespace-separated syntax is supported.
- The prompt retains Chloe's fixed hostname/current-directory buffers; an unusually
  long or inaccessible working directory can produce an empty directory field.
- The complete shell (Parts 0-9) has been verified and tested successfully after Max's integration, with all features fully functional.
- No known failures remained in the tested parts 0-9 after the documented fixes.

## Extra credit

None claimed. No timeout executable is required by the supplied current assignment.

## AI assistance

GPT assisted with planning, code review, Linux tests and documentation. Gemini also provided assistance for Max's contribution, helping to implement, seamlessly integrate, and comment Parts 7-9.

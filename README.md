# COP 4610 Project 1 shell

This local contribution implements parts 4-6 on top of Chloe Patrick's parts 0-3.


## Build and run

Use Linux (linprog or WSL Ubuntu), GCC, and GNU Make. The shell uses C99 and POSIX
interfaces, with no external libraries. Native Windows/MinGW is not the target.
From this directory:

```sh
make
./bin/shell
```

`make run` also starts the shell. Leave this partial build with Ctrl-D on an empty
input line. The `exit`, `cd`, and `jobs` built-ins belong to Max's part 9 and are not
implemented here. The prompt uses the current username, hostname, and directory.

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

Parts 0-3 were supplied in `shell_updated.tar`.

### Juan Medina Molina

| Date | Work completed |
| --- | --- |
| 2026-09-28 | Used GPT to review the supplied baseline, correct prerequisite defects, and verify the build in WSL Ubuntu and on linprog6. |

### Max Hackner



## Meetings

Various meetings throughout various weeks.

## Known limitations and unfinished work

- Parts 7-9 are unfinished in this folder. Pipes, background syntax, and built-ins
  currently produce explicit diagnostics; Max must integrate his implementations.
- Quotes, escapes, globs, adjacent operators, append redirects and full Bash grammar
  are not implemented. The required whitespace-separated syntax is supported.
- The prompt retains Chloe's fixed hostname/current-directory buffers; an unusually
  long or inaccessible working directory can produce an empty directory field.
- Juan verified this version on linprog6: warning-free build, all 19 integration tests,
  and execution API checks passed. The complete shell still needs testing after parts 7-9
  are integrated.
- No known failures remained in the tested parts 0-6 after the documented fixes.

## Extra credit

None claimed. No timeout executable is required by the supplied current assignment.

## AI assistance

GPT assisted with planning, code review, Linux tests and documentation.

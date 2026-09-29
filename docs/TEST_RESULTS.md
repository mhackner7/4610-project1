# Validation results

Date: 2026-09-28. Environment: WSL Ubuntu, normal non-root user.
Sources were copied into temporary Linux directories for builds and permission tests;
no executables or object files were added to this source handoff.

## Completed

- Original Chloe archive: reproduced GCC syntax failure on the trailing `~`.
- Normal C99 build: passed with `-Wall -Wextra -Wpedantic`, no warnings.
- 19 Python integration tests: passed, including permission checks (not skipped).
- C execution API tests: passed for true, false, unresolved command, failed input
  redirection, invalid executable format, and a real program exiting with 127.
- Full `make test` under AddressSanitizer and UndefinedBehaviorSanitizer: passed
  with `-Werror`, leak detection enabled and halt-on-error enabled. No sanitizer
  errors were reported.
- Repeated execution: 100 redirected commands passed with RLIMIT_NOFILE set to 32.
- Valgrind: not installed, so no Valgrind result is claimed.

The tests cover exact new/existing output permissions, truncation, both redirect
orders, preservation of input, rejection of same-inode aliases, nonregular input,
PATH ordering and empty components, arguments, expansions, prompt recovery, EOF,
and foreground waits. Fork/resource exhaustion is handled but not fault-injected.

## linprog verification and final integration check

Juan supplied the linprog6 terminal results on 2026-09-28: GCC compiled without
warnings, all 19 integration tests passed, the execution API checks passed, and
`./bin/shell` launched with the expected prompt. This was user-run verification;
the agent did not access Juan's linprog account.

To repeat these checks on linprog:

```sh
make clean
make
make test
./bin/shell
```

Interactively try `ls -al`, `echo $USER`, `echo ~`, `echo hello > result.txt`,
`cat < result.txt`, and `sort > sorted.txt < result.txt`. Use Ctrl-D to leave this
partial build. Verify `stat -c %a result.txt` reports 600. After Max integrates parts
7-9, additionally verify the complete course sample runs and built-ins, pipelines,
background jobs, and exit/history behavior. Update the README's unfinished-work list
and real meeting records before final submission. Run `make clean` and exclude any
manual-test output files and compiled artifacts from the final repository.

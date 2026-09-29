# Integration with Max's parts 7-9

The main loop currently calls `execute_external(tokens)` after tokenization, tilde
expansion and environment expansion. Insert built-in, pipeline and background
handling before that foreground fallback. Preserve `input` until history/job code
has copied the original command line. Built-ins must run in the parent as appropriate
and must not use execv; this contribution does not implement them.

## Interfaces and ownership

- `resolve_path(command)` returns an allocated executable pathname, or NULL with
  errno set. Free the result. The environment and original command are untouched.
- `parse_redirection(tokens, &redir, &arguments)` deep-copies filenames and ordinary
  arguments. On success, free with `free_redirection(&redir)` and
  `free_tokens(arguments)`. On failure it reports an error, frees partial results,
  clears redir, and leaves arguments NULL. Pass fresh output objects.
- Set arguments->items[0] to the resolved path, freeing its previous string. The
  argument list then owns the path. Its final pointer must remain NULL.
- `apply_redirection(&redir)` is child-only. It reports errors and returns 0 or -1.
  The caller must terminate a child on failure. It does not fork or wait.
- `exec_external_child(path, argv, &redir, report_fd)` applies redirects and calls
  execv. It never returns. Pass -1 when no error-report channel is needed.
- `spawn_external(path, argv, &redir, &report_fd)` forks without waiting, returning
  the PID or -1. It creates a close-on-exec error-report pipe, not a command pipeline.
  The caller owns report_fd after success. Arguments/redirection remain caller-owned
  and may be freed in the parent after successful fork.
- `wait_external(pid, report_fd)` retries interrupted waits, reads any child setup
  failure, closes report_fd and returns `execution_result`. Pass -1 if no channel
  exists. Call it only for a child that has not already been reaped.
- `execute_external(tokens)` provides parsing, resolution, spawning, waiting and
  cleanup for one foreground command. It leaves the input token list unchanged.

`execution_result.error` distinguishes preparation, spawning, waiting, redirection,
and exec failures. `EXECUTION_OK` means exec succeeded and wait completed, even if
`status` is nonzero. Status is the program exit code, 128+signal, or -1 if unavailable.
A close-on-exec report channel distinguishes exec failure from a program returning
126/127. Decide history policy in Max's part 9 rather than equating status 0 with a
valid command.

## Background commands

Remove the final `&` before parsing. Prepare the command and call spawn_external,
then retain its PID/report_fd in the job record without calling wait_external at
launch. Store a copy of the original command line and allocate Max's increasing job
number. Once waitpid(WNOHANG) reaps that PID, inspect/close the saved report descriptor
and normalize its status yourself; do not call wait_external on an already-reaped PID.
The report carries one execution_error value on child setup failure, or EOF on exec
success. Close every retained descriptor when its job is removed.

## Pipelines

Split at `|` before calling the command parser: it intentionally rejects unsplit `|`
and `&`. Resolve each command, create data pipes, fork every stage, connect pipe
endpoints with dup2, and close all unneeded pipe ends before calling
exec_external_child in each already-forked child. Use an empty redirection structure
for ordinary pipelines. Do not use the foreground wrapper for each stage, which
would wait too early and can deadlock. The parent closes its pipe ends and reaps all
children. Data-pipe ownership, job numbering and built-ins remain Max's work.

The assignment does not require combined piping/redirection; no extra credit is
claimed. Merge source/header files directly, retain Chloe's attribution, and replace
temporary unsupported-feature diagnostics as Max's dispatch is connected.

## Pending Chloe pull request

This contribution includes the source extracted from Chloe Patrick's unmerged
`chloe-parts-0-3` archive (upstream PR #1), with the fixes listed in CHLOE_REVIEW.md.
Credit Chloe for parts 0-3 and Juan for parts 4-6 and the documented corrections.
Coordinate PR #1 with this PR: the extracted source here replaces the need to retain
the archive as the buildable project. The final repository should build from its root.

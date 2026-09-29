# Chloe baseline review

Reviewed the local `shell_updated.tar` against the archive on Chloe's
`chloe-parts-0-3` branch. They have the same SHA-256:

```text
52fd81cbf1d1855485aa96e3b4c9e714afa1fa5b6cd2fcd1d5278967c4eae4dd
```

Source: https://github.com/Chloe-Patrick/4610-project1/tree/chloe-parts-0-3

## Findings and corrections

1. **Confirmed compilation failure:** `src/shell.c:136` ended with `}~`.
   GCC rejected the stray token. Removed only the trailing `~` in that file.
2. **Inherited input-reader defects:** the starter copied four bytes for a final
   short read without a newline, rather than the number actually read. Blank input
   could also take a zero-size allocation/null-pointer path. Replaced reading with
   a dynamically growing, checked character reader; NULL now signals EOF/read error.
   Updated main to use that contract and check read errors.
3. **Allocation handling:** added checked lexer allocations and NULL-safe cleanup.
   Tokens remain owned copies and the array remains NULL-terminated. Added tab/CR
   whitespace delimiters alongside spaces without introducing a new lexer grammar.
4. **Expansion order:** main now expands literal tildes before environment variables.
   A variable whose value is `~` remains that value instead of being expanded twice.
   Chloe's two expansion functions are otherwise unchanged.
5. **Build integration:** flattened the project layout and replaced the starter
   Makefile with directory prerequisites, header dependencies, warning flags, test
   targets and idempotent cleanup.

## Parts 1-3 assessment

The prompt obtains USER with getenv, the machine name with gethostname, and the
actual directory with getcwd. This produces the required format and avoids stale PWD
values after Max adds cd. Environment expansion replaces whole `$NAME` tokens,
including undefined variables with empty strings. Tilde expansion handles exactly
`~` and `~/...` and leaves other tilde forms unchanged.

The post-fix Linux tests verified these behaviors, empty/blank input, and final
commands without a newline. This is not a claim that the original archive compiled:
its compilation failure was reproduced before any correction.

Chloe's functions retain fixed prompt buffers and their existing allocation-failure
policy. The original archives are unchanged, and the new execution/redirection/PATH
modules are Juan's contribution with GPT assistance.

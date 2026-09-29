#pragma once
#include "redirection.h"
#include <sys/types.h>

typedef enum {
    EXECUTION_OK, PREPARATION_FAILED, SPAWN_FAILED, WAIT_FAILED,
    REDIRECTION_FAILED, EXEC_FAILED
} execution_error;

typedef struct {
    execution_error error;
    int status; /* Exit code, or 128 + signal; -1 if no child status is available. */
} execution_result;

/* Child only, never returns. report_fd may be -1 if no error report is needed. */
void exec_external_child(const char *path, char *const argv[],
                         const redirection *redir, int report_fd);
/* Caller owns *report_fd; wait_external closes it. Returns -1 on spawn failure. */
pid_t spawn_external(const char *path, char *const argv[],
                     const redirection *redir, int *report_fd);
execution_result wait_external(pid_t pid, int report_fd);
execution_result execute_external(const tokenlist *tokens);

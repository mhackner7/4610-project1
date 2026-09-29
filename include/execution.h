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

/* --- Added for Parts 7, 8, 9 --- */
void check_background_jobs(void);
void execute_builtin_cd(const tokenlist *tokens);
void execute_builtin_jobs(void);
void execute_builtin_exit(char history[3][256], int history_count);
void execute_pipeline(tokenlist **cmds, int num_cmds, int is_bg, const char *raw_cmd);

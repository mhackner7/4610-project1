#include "execution.h"
#include "path_search.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void child_failed(int fd, execution_error error, int status)
{
    if (fd >= 0) {
        ssize_t written;
        do {
            written = write(fd, &error, sizeof(error));
        } while (written < 0 && errno == EINTR);
        close(fd);
    }
    _exit(status);
}

void exec_external_child(const char *path, char *const argv[],
                         const redirection *redir, int report_fd)
{
    if (apply_redirection(redir) == -1)
        child_failed(report_fd, REDIRECTION_FAILED, 1);
    execv(path, argv);
    int failure = errno;
    fprintf(stderr, "%s: %s\n", path, strerror(failure));
    child_failed(report_fd, EXEC_FAILED, failure == ENOENT ? 127 : 126);
}

pid_t spawn_external(const char *path, char *const argv[],
                     const redirection *redir, int *report_fd)
{
    int channel[2];
    *report_fd = -1;
    if (pipe(channel) == -1)
        return -1;
    if (fcntl(channel[0], F_SETFD, FD_CLOEXEC) == -1 ||
        fcntl(channel[1], F_SETFD, FD_CLOEXEC) == -1) {
        int failure = errno;
        close(channel[0]);
        close(channel[1]);
        errno = failure;
        return -1;
    }
    fflush(NULL);
    pid_t pid = fork();
    if (pid == 0) {
        close(channel[0]);
        exec_external_child(path, argv, redir, channel[1]);
    }
    int failure = errno;
    close(channel[1]);
    if (pid == -1) {
        close(channel[0]);
        errno = failure;
        return -1;
    }
    *report_fd = channel[0];
    return pid;
}

execution_result wait_external(pid_t pid, int report_fd)
{
    execution_result result = {EXECUTION_OK, -1};
    int status;
    pid_t waited;
    do {
        waited = waitpid(pid, &status, 0);
    } while (waited == -1 && errno == EINTR);
    if (waited == -1) {
        perror("waitpid");
        result.error = WAIT_FAILED;
    } else {
        result.status = WIFEXITED(status) ? WEXITSTATUS(status) :
                        128 + WTERMSIG(status);
        if (report_fd >= 0) {
            execution_error child_error;
            ssize_t count;
            do {
                count = read(report_fd, &child_error, sizeof(child_error));
            } while (count == -1 && errno == EINTR);
            if (count == sizeof(child_error))
                result.error = child_error;
            else if (count != 0) {
                fprintf(stderr, "Unable to read child execution report.\n");
                result.error = WAIT_FAILED;
            }
        }
    }
    if (report_fd >= 0)
        close(report_fd);
    return result;
}

execution_result execute_external(const tokenlist *tokens)
{
    execution_result result = {PREPARATION_FAILED, -1};
    redirection redir;
    tokenlist *arguments;
    if (!parse_redirection(tokens, &redir, &arguments))
        return result;
    const char *command = arguments->items[0];
    if (strcmp(command, "cd") == 0 || strcmp(command, "exit") == 0 ||
        strcmp(command, "jobs") == 0) {
        fprintf(stderr, "%s: built-in awaits part 9; use EOF to leave this build.\n",
                command);
        goto cleanup;
    }
    char *path = resolve_path(command);
    if (path == NULL) {
        fprintf(stderr, "%s: %s\n", command,
                errno == ENOENT ? "command not found" : strerror(errno));
        goto cleanup;
    }
    free(arguments->items[0]);
    arguments->items[0] = path;
    int report_fd;
    pid_t pid = spawn_external(path, arguments->items, &redir, &report_fd);
    if (pid == -1) {
        perror("spawn external command");
        result.error = SPAWN_FAILED;
    } else {
        result = wait_external(pid, report_fd);
    }
cleanup:
    free_tokens(arguments);
    free_redirection(&redir);
    return result;
}

// ==========================================
// Parts 7, 8, 9 Implementation
// ==========================================

#include <sys/stat.h>

#define MAX_JOBS 10
typedef struct {
    int job_id;
    pid_t pid;
    char cmd_line[256];
    int active;
} BackgroundJob;

static BackgroundJob jobs[MAX_JOBS];
static int next_job_id = 1;

/*
 * check_background_jobs (Part 8)
 * Iterates through the active jobs array. Uses waitpid with WNOHANG
 * to silently check if any background processes have finished.
 * Outputs the completion message if they have.
 */
void check_background_jobs(void) {
    int status;
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].active) {
            pid_t result = waitpid(jobs[i].pid, &status, WNOHANG);
            if (result > 0) {
                printf("[%d]+ done %s\n", jobs[i].job_id, jobs[i].cmd_line);
                jobs[i].active = 0;
            } else if (result == -1) {
                jobs[i].active = 0;
            }
        }
    }
}

/*
 * execute_builtin_cd (Part 9)
 * Built-in for changing directories. Defaults to $HOME if no args.
 * Validates existence and checks if target is a directory.
 */
void execute_builtin_cd(const tokenlist *tokens) {
    const char *target = NULL;
    if (tokens->size > 1) target = tokens->items[1];
    
    if (tokens->size > 2) {
        fprintf(stderr, "cd: too many arguments\n");
        return;
    }
    
    if (target == NULL) {
        target = getenv("HOME");
    }
    
    struct stat statbuf;
    if (stat(target, &statbuf) != 0) {
        fprintf(stderr, "cd: %s: No such file or directory\n", target);
        return;
    }
    if (!S_ISDIR(statbuf.st_mode)) {
        fprintf(stderr, "cd: %s: Not a directory\n", target);
        return;
    }
    
    if (chdir(target) != 0) {
        perror("cd failed");
    }
}

/*
 * execute_builtin_jobs (Part 9)
 * Iterates over the background jobs array and prints active jobs.
 */
void execute_builtin_jobs(void) {
    int count = 0;
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].active) {
            printf("[%d]+ %d %s\n", jobs[i].job_id, jobs[i].pid, jobs[i].cmd_line);
            count++;
        }
    }
    if (count == 0) {
        printf("No active background jobs.\n");
    }
}

/*
 * execute_builtin_exit (Part 9)
 * Waits for all background processes to gracefully exit,
 * prints the rolling history of valid commands, then terminates the shell.
 */
void execute_builtin_exit(char history[3][256], int history_count) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].active) {
            waitpid(jobs[i].pid, NULL, 0);
        }
    }
    
    if (history_count == 0) {
        printf("No valid commands in history.\n");
    } else {
        printf("Last valid commands:\n");
        for (int i = 0; i < history_count; i++) {
            printf("%d: %s\n", i + 1, history[i]);
        }
    }
    exit(0);
}

/*
 * execute_pipeline (Part 7, 8)
 * Handles sequential piping (up to 3 commands / 2 pipes), applies
 * external file redirection to each via `exec_external_child`, 
 * and handles background execution monitoring.
 */
void execute_pipeline(tokenlist **cmds, int num_cmds, int is_bg, const char *raw_cmd) {
    int pipes[2][2];
    pid_t pids[3];

    for (int i = 0; i < num_cmds - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe failed");
            return;
        }
    }

    for (int i = 0; i < num_cmds; i++) {
        redirection redir;
        tokenlist *arguments;
        if (!parse_redirection(cmds[i], &redir, &arguments)) {
            continue;
        }

        char *path = resolve_path(arguments->items[0]);
        if (path == NULL) {
            fprintf(stderr, "%s: %s\n", arguments->items[0],
                    errno == ENOENT ? "command not found" : strerror(errno));
            free_tokens(arguments);
            free_redirection(&redir);
            continue;
        }
        
        free(arguments->items[0]);
        arguments->items[0] = path;

        pids[i] = fork();
        if (pids[i] == -1) {
            perror("fork failed");
            free_tokens(arguments);
            free_redirection(&redir);
            return;
        }
        
        if (pids[i] == 0) {
            if (i > 0) dup2(pipes[i-1][0], STDIN_FILENO);
            if (i < num_cmds - 1) dup2(pipes[i][1], STDOUT_FILENO);

            for (int j = 0; j < num_cmds - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            exec_external_child(path, arguments->items, &redir, -1);
        }

        free_tokens(arguments);
        free_redirection(&redir);
    }

    for (int i = 0; i < num_cmds - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    if (is_bg && num_cmds > 0) {
        int slot = -1;
        for (int i = 0; i < MAX_JOBS; i++) {
            if (!jobs[i].active) { slot = i; break; }
        }
        if (slot != -1) {
            jobs[slot].job_id = next_job_id++;
            jobs[slot].pid = pids[num_cmds - 1]; 
            strncpy(jobs[slot].cmd_line, raw_cmd, 255);
            jobs[slot].cmd_line[255] = '\0';
            jobs[slot].active = 1;
            printf("[%d] %d\n", jobs[slot].job_id, jobs[slot].pid);
        } else {
            fprintf(stderr, "Max background jobs reached.\n");
        }
    } else {
        for (int i = 0; i < num_cmds; i++) {
            waitpid(pids[i], NULL, 0);
        }
    }
}

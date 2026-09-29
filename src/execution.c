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

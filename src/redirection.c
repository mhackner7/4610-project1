#include "redirection.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static bool operator_token(const char *word)
{
    return strcmp(word, "<") == 0 || strcmp(word, ">") == 0 ||
           strcmp(word, "|") == 0 || strcmp(word, "&") == 0;
}

void free_redirection(redirection *redir)
{
    free(redir->input_file);
    free(redir->output_file);
    *redir = (redirection){0};
}

bool parse_redirection(const tokenlist *tokens, redirection *redir,
                       tokenlist **argv_out)
{
    *redir = (redirection){0};
    *argv_out = NULL;
    tokenlist *arguments = new_tokenlist();
    for (size_t i = 0; i < tokens->size; i++) {
        char *word = tokens->items[i];
        if (strcmp(word, "|") == 0 || strcmp(word, "&") == 0) {
            fprintf(stderr, "Piping and background jobs await parts 7-8.\n");
            goto fail;
        }
        if (strcmp(word, "<") != 0 && strcmp(word, ">") != 0) {
            add_token(arguments, word);
            continue;
        }
        char **filename = strcmp(word, "<") == 0 ?
                          &redir->input_file : &redir->output_file;
        if (*filename != NULL || i + 1 == tokens->size ||
            operator_token(tokens->items[i + 1]) || tokens->items[i + 1][0] == '\0') {
            fprintf(stderr, "Invalid %s redirection: expected one filename.\n", word);
            goto fail;
        }
        *filename = strdup(tokens->items[++i]);
        if (*filename == NULL) {
            perror("redirection allocation");
            goto fail;
        }
    }
    if (arguments->size == 0 || arguments->items[0][0] == '\0') {
        fprintf(stderr, "Missing command.\n");
        goto fail;
    }
    *argv_out = arguments;
    return true;
fail:
    free_tokens(arguments);
    free_redirection(redir);
    return false;
}

int apply_redirection(const redirection *redir)
{
    int input = -1, output = -1;
    struct stat in_info, out_info;
    const char *context = redir->input_file;
    if (redir->input_file != NULL) {
        if (stat(redir->input_file, &in_info) == -1)
            goto fail;
        if (!S_ISREG(in_info.st_mode)) {
            errno = EINVAL;
            goto fail;
        }
        /* Nonblocking protects against a path being replaced with a FIFO. */
        input = open(redir->input_file, O_RDONLY | O_NONBLOCK);
        if (input == -1 || fstat(input, &in_info) == -1)
            goto fail;
        if (!S_ISREG(in_info.st_mode)) {
            errno = EINVAL;
            goto fail;
        }
    }
    if (redir->output_file != NULL) {
        context = redir->output_file;
        /* Delay truncation until after same-file and permission checks. */
        output = open(redir->output_file, O_WRONLY | O_CREAT | O_NONBLOCK, 0600);
        if (output == -1 || fstat(output, &out_info) == -1)
            goto fail;
        if (!S_ISREG(out_info.st_mode)) {
            errno = EINVAL;
            goto fail;
        }
        if (input != -1 && in_info.st_dev == out_info.st_dev &&
            in_info.st_ino == out_info.st_ino) {
            fprintf(stderr, "Input and output refer to the same file.\n");
            goto cleanup;
        }
        if (fchmod(output, 0600) == -1 || ftruncate(output, 0) == -1)
            goto fail;
    }
    context = "redirect stdin";
    if (input != -1 && dup2(input, STDIN_FILENO) == -1)
        goto fail;
    context = "redirect stdout";
    if (output != -1 && dup2(output, STDOUT_FILENO) == -1)
        goto fail;
    if (input > STDERR_FILENO)
        close(input);
    if (output > STDERR_FILENO)
        close(output);
    return 0;
fail:
    perror(context);
cleanup:
    if (input != -1)
        close(input);
    if (output != -1)
        close(output);
    return -1;
}

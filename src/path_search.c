#include "path_search.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int executable_file(const char *path)
{
    struct stat info;
    if (stat(path, &info) == -1)
        return 0;
    if (!S_ISREG(info.st_mode)) {
        errno = EACCES;
        return 0;
    }
    return access(path, X_OK) == 0;
}

char *resolve_path(const char *command)
{
    if (command == NULL || command[0] == '\0') {
        errno = ENOENT;
        return NULL;
    }
    if (strchr(command, '/') != NULL)
        return executable_file(command) ? strdup(command) : NULL;
    const char *path = getenv("PATH");
    if (path == NULL) {
        errno = ENOENT;
        return NULL;
    }
    int saved_error = ENOENT;
    const char *start = path;
    do {
        const char *end = strchr(start, ':');
        size_t length = end ? (size_t)(end - start) : strlen(start);
        const char *directory = length ? start : ".";
        if (length == 0)
            length = 1;
        char *candidate = malloc(length + strlen(command) + 2);
        if (candidate == NULL)
            return NULL;
        memcpy(candidate, directory, length);
        candidate[length] = '/';
        strcpy(candidate + length + 1, command);
        if (executable_file(candidate))
            return candidate;
        if (errno != ENOENT && errno != ENOTDIR)
            saved_error = errno;
        free(candidate);
        if (end == NULL)
            break;
        start = end + 1;
    } while (1);
    errno = saved_error;
    return NULL;
}

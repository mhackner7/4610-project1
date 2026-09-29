#pragma once

/* Caller frees the result. Failure returns NULL and sets errno. */
char *resolve_path(const char *command);

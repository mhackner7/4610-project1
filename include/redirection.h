#pragma once
#include "lexer.h"

typedef struct {
    char *input_file;
    char *output_file;
} redirection;

/* Deep-copies arguments and paths. Failure cleans and empties both outputs. */
bool parse_redirection(const tokenlist *tokens, redirection *redir,
                       tokenlist **argv_out);
void free_redirection(redirection *redir);
/* Child only: prints errors, returns -1 on failure, 0 on success. */
int apply_redirection(const redirection *redir);

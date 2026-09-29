#define _POSIX_C_SOURCE 200809L

#include "shell.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



static void replace_token(tokenlist *tokens, size_t index,
                          const char *replacement)
{
    char *new_token =
        (char *)malloc(strlen(replacement) + 1);

    if (new_token == NULL)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    strcpy(new_token, replacement);

    free(tokens->items[index]);
    tokens->items[index] = new_token;
}



static bool is_environment_variable(const char *token)
{
    if (token[0] != '$' || token[1] == '\0')
        return false;

    for (int i = 1; token[i] != '\0'; i++)
    {
        if (!isalnum((unsigned char)token[i]) &&
            token[i] != '_')
        {
            return false;
        }
    }

    return true;
}



void print_prompt(void)
{
    char hostname[256];
    char cwd[4096];

    char *user = getenv("USER");

    if (user == NULL)
        user = "";

    if (gethostname(hostname, sizeof(hostname)) != 0)
    {
        hostname[0] = '\0';
    }

    hostname[sizeof(hostname) - 1] = '\0';

    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        cwd[0] = '\0';
    }

    printf("%s@%s:%s>", user, hostname, cwd);
    fflush(stdout);
}



void expand_environment_variables(tokenlist *tokens)
{
    for (size_t i = 0; i < tokens->size; i++)
    {
        char *token = tokens->items[i];

        if (!is_environment_variable(token))
            continue;

        char *value = getenv(token + 1);

        if (value == NULL)
            value = "";

        replace_token(tokens, i, value);
    }
}



void expand_tilde(tokenlist *tokens)
{
    char *home = getenv("HOME");

    if (home == NULL)
        return;

    for (size_t i = 0; i < tokens->size; i++)
    {
        char *token = tokens->items[i];

        if (strcmp(token, "~") == 0)
        {
            replace_token(tokens, i, home);
        }
        else if (strncmp(token, "~/", 2) == 0)
        {
            size_t length =
                strlen(home) + strlen(token + 1) + 1;

            char *expanded =
                (char *)malloc(length);

            if (expanded == NULL)
            {
                perror("malloc");
                exit(EXIT_FAILURE);
            }

            strcpy(expanded, home);
            strcat(expanded, token + 1);

            free(tokens->items[i]);
            tokens->items[i] = expanded;
        }
    }
}

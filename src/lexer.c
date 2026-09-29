#include "lexer.h"
#include <stdio.h>
#include <string.h>

static void *checked_realloc(void *memory, size_t size)
{
    void *result = realloc(memory, size);
    if (result == NULL) {
        perror("allocation");
        free(memory);
        exit(EXIT_FAILURE);
    }
    return result;
}

/* NULL means EOF/read failure; an allocated empty string means a blank line. */
char *get_input(void)
{
    size_t size = 0, capacity = 128;
    char *buffer = checked_realloc(NULL, capacity);
    int ch;
    while ((ch = getchar()) != EOF && ch != '\n') {
        if (size + 1 >= capacity) {
            capacity *= 2;
            buffer = checked_realloc(buffer, capacity);
        }
        buffer[size++] = (char)ch;
    }
    if (ferror(stdin) || (ch == EOF && size == 0)) {
        free(buffer);
        return NULL;
    }
    buffer[size] = '\0';
    return buffer;
}

tokenlist *new_tokenlist(void)
{
    tokenlist *tokens = checked_realloc(NULL, sizeof(*tokens));
    tokens->size = 0;
    tokens->items = checked_realloc(NULL, sizeof(*tokens->items));
    tokens->items[0] = NULL;
    return tokens;
}

void add_token(tokenlist *tokens, char *item)
{
    size_t index = tokens->size;
    tokens->items = checked_realloc(tokens->items, (index + 2) * sizeof(char *));
    tokens->items[index] = checked_realloc(NULL, strlen(item) + 1);
    strcpy(tokens->items[index], item);
    tokens->items[index + 1] = NULL;
    tokens->size++;
}

tokenlist *get_tokens(char *input)
{
    char *copy = checked_realloc(NULL, strlen(input) + 1);
    strcpy(copy, input);
    tokenlist *tokens = new_tokenlist();
    for (char *word = strtok(copy, " \t\r"); word != NULL;
         word = strtok(NULL, " \t\r"))
        add_token(tokens, word);
    free(copy);
    return tokens;
}

void free_tokens(tokenlist *tokens)
{
    if (tokens == NULL)
        return;
    for (size_t i = 0; i < tokens->size; i++)
        free(tokens->items[i]);
    free(tokens->items);
    free(tokens);
}

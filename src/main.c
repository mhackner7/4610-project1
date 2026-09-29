#include "lexer.h"
#include "shell.h"
#include "execution.h"

#include <stdio.h>
#include <stdlib.h>


int main(void)
{
    while (1)
    {
        print_prompt();

        char *input = get_input();

        if (input == NULL)
        {
            if (ferror(stdin)) {
                perror("read input");
                return EXIT_FAILURE;
            }
            printf("\n");
            break;
        }

        tokenlist *tokens = get_tokens(input);

        expand_tilde(tokens);
        expand_environment_variables(tokens);

        /* Max: dispatch built-ins, pipes and background jobs here first.
         * Keep input intact until job/history handling has copied it.
         */
        if (tokens->size > 0)
            execute_external(tokens);

        free(input);
        free_tokens(tokens);
    }

    return 0;
}

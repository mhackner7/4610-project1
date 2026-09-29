#include "lexer.h"
#include "shell.h"
#include "execution.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Array and counter to store the last 3 valid commands for the exit built-in */
char history[3][256];
int history_count = 0;

/*
 * add_history
 * Saves a valid command to a rolling 3-command history buffer.
 * Used exclusively by the `exit` command built-in (Part 9).
 */
void add_history(const char *cmd) {
    if (history_count < 3) {
        strncpy(history[history_count], cmd, 255);
        history[history_count][255] = '\0';
        history_count++;
    } else {
        strncpy(history[0], history[1], 256);
        strncpy(history[1], history[2], 256);
        strncpy(history[2], cmd, 255);
        history[2][255] = '\0';
    }
}

int main(void)
{
    while (1)
    {
        check_background_jobs();
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

        char raw_cmd[1024];
        strncpy(raw_cmd, input, 1023);
        raw_cmd[1023] = '\0';
        raw_cmd[strcspn(raw_cmd, "\n")] = '\0';

        tokenlist *tokens = get_tokens(input);

        expand_tilde(tokens);
        expand_environment_variables(tokens);

        if (tokens->size > 0) {
            if (strlen(raw_cmd) > 0) {
                add_history(raw_cmd);
            }

            /*
             * Part 8: Background Parsing
             * Detect trailing '&', mark the job as background, and strip the token.
             */
            int is_bg = 0;
            if (strcmp(tokens->items[tokens->size - 1], "&") == 0) {
                is_bg = 1;
                free(tokens->items[tokens->size - 1]);
                tokens->size--;
            }

            /*
             * Part 7: Pipeline Parsing
             * Splits the main tokenlist into an array of tokenlists (max 2 pipes).
             */
            tokenlist *pipelines[3];
            int num_cmds = 1;
            pipelines[0] = new_tokenlist();

            for (size_t i = 0; i < tokens->size; i++) {
                if (strcmp(tokens->items[i], "|") == 0) {
                    if (num_cmds < 3) {
                        num_cmds++;
                        pipelines[num_cmds - 1] = new_tokenlist();
                    }
                } else {
                    char *tok_copy = (char*)malloc(strlen(tokens->items[i]) + 1);
                    strcpy(tok_copy, tokens->items[i]);
                    add_token(pipelines[num_cmds - 1], tok_copy);
                }
            }

            /*
             * Part 9 & Execution Routing
             * If single command, intercept built-ins. Else, route to execute_pipeline.
             */
            if (num_cmds == 1 && pipelines[0]->size > 0) {
                const char *cmd = pipelines[0]->items[0];
                if (strcmp(cmd, "exit") == 0) {
                    execute_builtin_exit(history, history_count);
                } else if (strcmp(cmd, "cd") == 0) {
                    execute_builtin_cd(pipelines[0]);
                } else if (strcmp(cmd, "jobs") == 0) {
                    execute_builtin_jobs();
                } else {
                    execute_pipeline(pipelines, num_cmds, is_bg, raw_cmd);
                }
            } else if (num_cmds > 0 && pipelines[0]->size > 0) {
                execute_pipeline(pipelines, num_cmds, is_bg, raw_cmd);
            }

            for (int i = 0; i < num_cmds; i++) {
                free_tokens(pipelines[i]);
            }
        }

        free(input);
        free_tokens(tokens);
    }

    return 0;
}

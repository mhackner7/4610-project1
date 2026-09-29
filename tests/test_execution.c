#include "execution.h"
#include <assert.h>
#include <stdio.h>

static execution_result run(char *line)
{
    tokenlist *tokens = get_tokens(line);
    execution_result result = execute_external(tokens);
    free_tokens(tokens);
    return result;
}

int main(void)
{
    execution_result result = run("/bin/false");
    assert(result.error == EXECUTION_OK && result.status == 1);
    result = run("/bin/true");
    assert(result.error == EXECUTION_OK && result.status == 0);
    result = run("/definitely_missing_4610");
    assert(result.error == PREPARATION_FAILED && result.status == -1);
    result = run("/bin/cat < /definitely_missing_4610");
    assert(result.error == REDIRECTION_FAILED && result.status == 1);
    result = run("./bad-executable");
    assert(result.error == EXEC_FAILED && result.status == 126);
    result = run("./exit127");
    assert(result.error == EXECUTION_OK && result.status == 127);
    puts("Execution result distinctions passed.");
    return 0;
}

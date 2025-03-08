#include <sylvan/command_handler.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief handles the input commands by the users
 * @param [in] command command provided by the user
 * @param [in] inf the inferior which the user is working on
 * @return 0 if success 1 if failure   
 */

extern int handle_command(char *command, struct inferior *inf)
{
    printf("%s\n", command);

    if(strcmp(command, "exit") == 0)
    {
        return 1;
    }
    return 0;
}

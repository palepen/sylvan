#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

#include "handle_command.h"
#include "sylvan/inferior.h"

static size_t djb2_hash(char *str)
{
    size_t hash = 5381;
    int c;

    while ((c = *str++))
    {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}



struct command_data commands_available[] =
{
    #define DEFINE_COMMAND(name, desc, t)  \
    {name, desc, 0, t}
    #include "details/commands.inc"
    #undef DEFINE_REGISTER
    {NULL, NULL, 0, 0}
};

int create_hash()
{
    size_t i = 0;
    while (commands_available[i].name && commands_available[i].desc)
    {
        commands_available[i].hash = djb2_hash(commands_available[i].name);
        i++;
    }
    return 0;
}


static void print_commands(enum command_type tp)
{
    printf("Commands:\n");
    size_t i = 0;
    while (commands_available[i].name && commands_available[i].desc)
    {
        
        if (commands_available[i].cmd_type != tp)
        {
            i++;
            continue;
        }
    
        printf("    %s  - %s\n", commands_available[i].name, commands_available[i].desc);
        i++;
    }
}

/**
 * @brief handles the input commands by the users
 * @param [in] command command provided by the user
 * @param [in] inf the sylvan_inferior which the user is working on
 * @return 0 if success 1 if failure   
 */

int handle_command(char **command, struct sylvan_inferior *inf)
{


    size_t hash_cmd = djb2_hash(command[0]);

    if (commands_available[0].hash == hash_cmd)
    {
        print_commands(SYLVAN_STANDARD_COMMAND);
    }
    else if (commands_available[1].hash == hash_cmd)
    {
        
        printf("Exiting Debugger\n");
        return 1;
    }
    else if (commands_available[2].hash == hash_cmd)
    {
        if (inf->status != SYLVAN_INFSTATE_STOPPED)
        {
            printf("Process is not stopped\n");
        }
        else if (sylvan_continue(inf) < 0)
        {
            
            fprintf(stderr, "Failed to continue process\n");
            return 1;
        }
    }
    else if(commands_available[4].hash == hash_cmd)
    {
        if(command[1] == NULL)
            print_commands(SYLVAN_INFO_COMMAND);
        

        
        
    }
    else
    {

        printf("Unknown command: %s\n", command);
        printf("Type 'help' for available commands\n");
    }

    return 0;
}

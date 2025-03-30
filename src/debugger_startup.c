#include <getopt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "debugger_startup.h"
#include "sylvan/inferior.h"
#include "cmd.h"
#include "user_interface.h"

extern volatile sig_atomic_t interrupted = 0; 

void cmd_args_init(struct cmd_args *args)
{
    struct cmd_args tmp = {
        .file_args = NULL,
        .filepath = NULL,
        .is_attached = false,
        .pid = 0,
    };
    *args = tmp;
}

void print_help()
{
    printf("Usage: sylvan [options]\n");
    printf("  -h, --help                Show this help message\n");
    printf("  -v, --version             Show version information\n");
    printf("  -a, --args   <args>       Pass <args> as arguments to the sylvan_inferior\n");
    printf("  -p, --attach <pid>        Attach to a process with process id <pid>\n");
}

void print_version()
{
    printf("Version 0.1.0\n");
}

void print_invalid_opt()
{
    fprintf(stderr, "Use 'sylvan -h or --help' to print a list of options.\n");
}

void parse_args(int argc, char *argv[], struct cmd_args *cmd_args)
{

    struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {"args", required_argument, NULL, 'a'},
        {"attach", required_argument, NULL, 'p'},
        {0, 0, 0, 0},
    };

    char opt;

    while ((opt = getopt_long(argc, argv, "hva:p:", long_options, NULL)) != -1)
    {
        switch (opt)
        {

        case 'h':
            print_help();
            exit(EXIT_SUCCESS);
        case 'v':
            print_version();
            exit(EXIT_SUCCESS);

        case 'a':
            cmd_args->file_args = optarg;
            break;
        case 'p':
        {
            char *end;
            pid_t pid = strtol(optarg, &end, 10);
            if (*end != '\0')
            {
                fprintf(stderr, "Invalid process id: %s\n", optarg);
                exit(EXIT_FAILURE);
            }
            cmd_args->is_attached = true;
            cmd_args->pid = pid;
            break;
        }
        case '?':
            print_invalid_opt();
            exit(EXIT_FAILURE);
        default:
            exit(EXIT_FAILURE);
        }
    }

    if (optind < argc)
        if (strcmp(argv[optind], "--") == 0)
            optind++;

    if (optind < argc)
        cmd_args->filepath = argv[optind++];

    if (optind < argc)
    {
        fprintf(stderr, "Ignoring excess options: ");
        while (optind < argc)
            fprintf(stderr, "%s ", argv[optind++]);
        fprintf(stderr, "\n");
    }
}

void error(const char *msg)
{
    fprintf(stderr, msg);
    fprintf(stderr, "\n");
}


static void handle_sigint(int sig)
{
    (void)sig;
    interrupted = 1;

}

int debugger_main(int argc, char **argv)
{
    struct cmd_args cmd_args = {0};
    struct sylvan_inferior *inf = NULL;

    if (signal(SIGINT, handle_sigint) == SIG_ERR)
    {
        perror("Failed to set SIGINT handler");
        return EXIT_FAILURE;
    }

    parse_args(argc, argv, &cmd_args);

    if (sylvan_inferior_create(&inf))
    {
        error(sylvan_get_last_error());
        return EXIT_FAILURE;
    }

    if (cmd_args.filepath)
    {
        if (sylvan_set_filepath(inf, cmd_args.filepath))
        {
            error(sylvan_get_last_error());
        }
    }

    if (cmd_args.file_args)
    {
        if (sylvan_set_args(inf, cmd_args.file_args))
        {
            error(sylvan_get_last_error());
        }
    }

    if (cmd_args.is_attached)
    {
        if (sylvan_attach(inf, cmd_args.pid))
        {
            error(sylvan_get_last_error());
        }
    }
    
    interface_loop(&inf);

    if (sylvan_inferior_destroy(inf))
    {
        error(sylvan_get_last_error());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
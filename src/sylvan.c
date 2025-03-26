#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/user.h>
#include <sylvan/inferior.h>
#include "cmd.h"
#include "user_interface.h"

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
    // exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{

    struct cmd_args cmd_args;
    cmd_args_init(&cmd_args);

    parse_args(argc, argv, &cmd_args);

    printf("running temporary program before the cli is ready\n");

    struct sylvan_inferior *inf;

    if (sylvan_inferior_create(&inf))
        error(sylvan_get_last_error());

    if (sylvan_set_args(inf, cmd_args.file_args))
        error(sylvan_get_last_error());

    if (cmd_args.filepath)
    {
        if (sylvan_set_filepath(inf, cmd_args.filepath))
            error(sylvan_get_last_error());
    }

    if (cmd_args.is_attached)
    {
        if (sylvan_attach(inf, cmd_args.pid))
            if (cmd_args.is_attached)
            {
                if (sylvan_attach_pid(inf, cmd_args.pid))
                    error(sylvan_get_last_error());
            }
            else if (inf->realpath)
            {
                printf("exec file: %s\n", inf->realpath);
            }
            else if (cmd_args.filepath)
            {
                if (sylvan_run(inf))
                    error(sylvan_get_last_error());
            }
    }
    
    interface_loop(&inf);
    return EXIT_SUCCESS;
}
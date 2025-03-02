#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>


struct cmd_args {
    const char *file_args;
    const char *filepath;
    bool attach_mode;
    pid_t pid;
};

void cmd_args_init(struct cmd_args *args) {
    struct cmd_args tmp = {
        .file_args = NULL,
        .filepath = NULL,
        .attach_mode = false,
        .pid = 0,
    };
    *args = tmp;
}

void print_help() {
    printf("Usage: sylvan [options]\n");
    printf("  -h, --help                Show this help message\n");
    printf("  -v, --version             Show version information\n");
    printf("  -a, --args   <args>       Pass <args> as arguments to the inferior\n");
    printf("  -p, --attach <pid>        Attach to a process with process id <pid>\n");
}

void print_version() {
    printf("Version 0.1.0\n");
}

void print_invalid_opt() {
    fprintf(stderr, "Use 'sylvan -h or --help' to print a list of options.\n");
}


void parse_args(int argc, char *argv[], struct cmd_args *cmd_args) {

    struct option long_options[] = {
        { "help",       no_argument,        NULL, 'h' },
        { "version",    no_argument,        NULL, 'v' },
        { "args",       required_argument,  NULL, 'a' },
        { "attach",     required_argument,  NULL, 'p' },
        { 0, 0, 0, 0 },
    };

    char opt;

    while ((opt = getopt_long(argc, argv, "hva:p:", long_options, NULL)) != -1) {
        switch (opt) {

            case 'h':
                print_help();
                exit(EXIT_SUCCESS);
            case 'v':
                print_version();
                exit(EXIT_SUCCESS);

            case 'a':
                cmd_args->file_args = optarg;
                break;
            case 'p': {
                char *end;
                pid_t pid = strtol(optarg, &end, 10);
                if (*end != '\0') {
                    fprintf(stderr, "Invalid process id: %s\n", optarg);
                    exit(EXIT_FAILURE);
                }
                cmd_args->attach_mode = true;
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

    if (optind < argc) {
        fprintf(stderr, "Ignoring excess options: ");
        while (optind < argc)
            fprintf(stderr, "%s ", argv[optind++]);
        fprintf(stderr, "\n");
    }

}

int main(int argc, char *argv[]) {

    struct cmd_args cmd_args;
    cmd_args_init(&cmd_args);

    parse_args(argc, argv, &cmd_args);

    return EXIT_SUCCESS;

}

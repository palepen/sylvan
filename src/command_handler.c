#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>
#include <elf.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "sylvan/inferior.h"
#include "command_handler.h"
#include "handle_command.h"
#include "auxv.h"
#include "sylvan/error.h"

/**
 * @brief Prints available commands or info subcommands
 * @param tp Type of commands to print (standard or info)
 */
static void print_commands(enum sylvan_command_type tp)
{
    size_t i = 0;
    if (tp == SYLVAN_STANDARD_COMMAND)
    {
        printf("Commands:\n");
        while (sylvan_commands[i].name && sylvan_commands[i].desc)
        {
            printf("    %s  - %s\n", sylvan_commands[i].name, sylvan_commands[i].desc);
            i++;
        }
    }
    else if (tp == SYLVAN_INFO_COMMAND)
    {
        printf("Info Commands:\n");
        while (sylvan_info_commands[i].name && sylvan_info_commands[i].desc)
        {
            printf("    info %s  - %s\n", sylvan_info_commands[i].name, sylvan_info_commands[i].desc);
            i++;
        }
    }
}

/** @brief Handler for 'help' command */
int handle_help(char **command, struct sylvan_inferior **inf)
{

    if (inf && command)
    {
        // supress warnings;
        (void)command;
        (void)inf;
    }

    print_commands(SYLVAN_STANDARD_COMMAND);
    return 0;
}

/** @brief Handler for 'quit' command */
int handle_exit(char **command, struct sylvan_inferior **inf)
{

    if (inf && command)
    {
        // supress warnings;
        (void)command;
        (void)inf;
    }

    printf("Exiting Debugger\n");
    return 1;
}

/** @brief Handler for 'continue' command */
int handle_continue(char **command, struct sylvan_inferior **inf)
{
    (void)command;
    if(!inf)
    {
        fprintf(stderr, "Null Inferior Pointer\n");
        return 0;
    }

    struct sylvan_inferior *curr_inf = *inf;
    if(!curr_inf)
    {
        fprintf(stderr, "Null Inferior Pointer\n");
        return 0;
    }
    if (curr_inf->status != SYLVAN_INFSTATE_STOPPED)
    {
        printf("Process is not stopped\n");
        return 0;
    }
    if (sylvan_continue(curr_inf) < 0)
    {
        fprintf(stderr, "Failed to continue process\n");
        return 1;
    }
    return 0;
}

/** @brief Handler for 'info' command */
int handle_info(char **command, struct sylvan_inferior **inf)
{

    if (inf && command)
    {
        // supress warnings;
        (void)command;
        (void)inf;
    }
    print_commands(SYLVAN_INFO_COMMAND);
    return 0;
}

/**
 * @brief Handler for info address command
 * @return 0 if success 1 for failure
 */
int handle_info_address(char **command, struct sylvan_inferior **inf)
{

    if (inf && command)
    {
        // supress warnings;
        (void)command;
        (void)inf;
    }

    printf("Symbol Table Required\n");
    return 0;
}

/**
 * @brief Handler for info all-address command
 * @return 0 if success 1 for failure
 */
int handle_info_all_registers(char **command, struct sylvan_inferior **inf)
{
    if (inf && command)
    {
        // supress warnings;
        (void)command;
        (void)inf;
    }

    printf("No Registers now\n");
    return 0;
}

int handle_info_args(char **command, struct sylvan_inferior **inf)
{

    if (inf && command)
    {
        // supress warnings;
        (void)command;
        (void)inf;
    }
    printf("Done by reading the registers\n");
    return 0;
}

int handle_info_auto_load(char **command, struct sylvan_inferior **inf)
{

    if (inf && command)
    {
        // supress warnings;
        (void)command;
        (void)inf;
    }
    printf("no auto-load support\n");
    return 0;
}

int handle_info_auxv(char **command, struct sylvan_inferior **inf)
{
    struct sylvan_inferior *curr_inf = *inf;
    
    // supress warnings;
    (void)command;

    if (!inf)
    {
        fprintf(stderr, "Null Inferior Pointer\n");
        return -1;
    }

    size_t len;
    unsigned char *raw_auxv = target_read_auxv(curr_inf, &len);

    if (!raw_auxv)
    {
        return -1;
    }

    struct auxv_entry *entries = parse_auxv(raw_auxv, len, 1);
    free(raw_auxv);

    if (!entries)
    {
        fprintf(stderr, "Error: Failed to parse auxv\n");
        return -1;
    }

    printf("Auxiliary Vector for PID %d:\n", curr_inf->pid);
    printf("Type  Value                 Name                Description\n");
    printf("----  --------------------  --------            -----------\n");
    for (size_t i = 0; entries[i].type != AT_NULL; i++)
    {
        print_auxv_entry(&entries[i]);
    }
    free(entries);
    return 0;
}

int handle_info_bookmark(char **command, struct sylvan_inferior **inf)
{
    if (!inf)
    {
        fprintf(stderr, "Error: Null Inferior Pointer\n");
        return -1;
    }

    if (inf && command)
    {
        // supress warnings;
        (void)command;
        (void)inf;
    }

    printf("Not Implemented\n");
    return 0;
}

int handle_info_breakpoints(char **command, struct sylvan_inferior **inf)
{
    if (inf && command)
    {
        // supress warnings;
        (void)command;
        (void)inf;
    }

    printf("Num     Type            Address             Status\n");
    printf("----    --------        ----------------    ------------\n");
    return 0;
}

int handle_info_copying(char **command, struct sylvan_inferior **inf)
{
    (void)command;
    (void)inf;

    printf("Sylvan Copying Conditions:\n");
    printf("This is a placeholder for Sylvan's redistribution terms.\n");
    return 0;
}

int handle_info_inferiors(char **command, struct sylvan_inferior **inf)
{
    struct sylvan_inferior *curr_inf = *inf;
    (void)command;
    if (!inf || !curr_inf)
    {
        return 0;
    }

    printf("Id: %d\n", curr_inf->id);
    printf("\tPID: %d\n", curr_inf->pid);
    printf("\tPath: %s\n", curr_inf->realpath);

    return 0;
}

int handle_add_inferior(char **command, struct sylvan_inferior **inf)
{
    if (!command[1])
    {
        printf("No file name provided\n");
        return 0;
    }
    
    
    sylvan_code_t ret = sylvan_inferior_create(inf);
    if (ret != SYLVANE_OK || !inf)
    {
        fprintf(stderr, sylvan_get_last_error());
        return 1;
    }
    
    const char *filepath = command[1];
    if (filepath)
    {
        ret = sylvan_set_filepath(*inf, filepath);
        if (ret != SYLVANE_OK)
        {
            fprintf(stderr, sylvan_get_last_error());
            sylvan_inferior_destroy(*inf);
            return 1;
        }
    }
    sylvan_run(*inf);
    struct sylvan_inferior *curr_inf = *inf;
    printf("Added inferior %d%s%s\n", curr_inf->pid, filepath ? " with executable " : "", filepath ? filepath : "");

    return 0;
}

int handle_objdump(char **command, struct sylvan_inferior **inf)
{
    struct sylvan_inferior *curr_inf = *inf;
    if (!inf || !curr_inf)
    {
        printf("Error: No current inferior provided.\n");
        return 0;
    }
    if (!curr_inf->realpath)
    {
        printf("Error: Current inferior has no executable set.\n");
        return 0;
    }

    int fd = open(curr_inf->realpath, O_RDONLY);
    if (fd < 0)
    {
        printf("Error: Cannot open file '%s'.\n", curr_inf->realpath);
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) < 0)
    {
        close(fd);
        printf("Error: Cannot stat file '%s'.\n", curr_inf->realpath);
        return 1;
    }
    void *map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED)
    {
        close(fd);
        printf("Error: Cannot mmap file '%s'.\n", curr_inf->realpath);
        return 1;
    }

    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)map;
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0)
    {
        munmap(map, st.st_size);
        close(fd);
        printf("Error: '%s' is not an ELF file.\n", curr_inf->realpath);
        return 1;
    }

    const char *option = command[1];
    if (!option)
    {
        printf("Usage: objdump [-h | -t]\n");
        printf("  -h: Display section headers\n");
        printf("  -t: Display symbol table\n");
    }
    else if (strcmp(option, "-h") == 0)
    {

        Elf64_Shdr *shdr = (Elf64_Shdr *)(map + ehdr->e_shoff);
        char *shstrtab = (char *)(map + shdr[ehdr->e_shstrndx].sh_offset);

        printf("Section Headers for '%s':\n", curr_inf->realpath);
        printf("  Num  Name                Size       Address\n");
        for (int i = 0; i < ehdr->e_shnum; i++)
        {
            printf("  %-3d  %-16s  %08lx  %016lx\n",
                   i,
                   shstrtab + shdr[i].sh_name,
                   (unsigned long)shdr[i].sh_size,
                   (unsigned long)shdr[i].sh_addr);
        }
    }
    else if (strcmp(option, "-t") == 0)
    {

        Elf64_Shdr *shdr = (Elf64_Shdr *)(map + ehdr->e_shoff);
        char *shstrtab = (char *)(map + shdr[ehdr->e_shstrndx].sh_offset);
        Elf64_Sym *symtab = NULL;
        int symtab_count = 0;
        char *strtab = NULL;

        for (int i = 0; i < ehdr->e_shnum; i++)
        {
            if (shdr[i].sh_type == SHT_SYMTAB)
            {
                symtab = (Elf64_Sym *)(map + shdr[i].sh_offset);
                symtab_count = shdr[i].sh_size / sizeof(Elf64_Sym);
                strtab = (char *)(map + shdr[shdr[i].sh_link].sh_offset);
                break;
            }
        }

        if (!symtab)
        {
            printf("No symbol table found in '%s'.\n", curr_inf->realpath);
        }
        else
        {
            printf("Symbol Table for '%s':\n", curr_inf->realpath);
            printf("  Value            Name\n");
            for (int i = 0; i < symtab_count; i++)
            {
                printf("  %016lx  %s\n",
                       (unsigned long)symtab[i].st_value,
                       strtab + symtab[i].st_name);
            }
        }
    }
    else
    {
        printf("Unknown option '%s'. Use -h or -t.\n", option);
    }

    munmap(map, st.st_size);
    close(fd);
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <Zydis/Zydis.h>
#include <string.h>
#include <stdint.h>
#include <libelf.h>
#include <gelf.h>
#include <elf.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "sylvan/inferior.h"
#include "disassemble.h"
#include "ui_utils.h"

static void free_instructions(struct disassembled_instruction *head)
{
    while (head)
    {
        struct disassembled_instruction *next = head->next;
        free(head->opcodes);
        free(head->instruction);
        free(head);
        head = next;
    }
}

/**
 * Parses the binary and then gets the offset for the virt address provided 
 */
static int get_file_offset_from_vaddr(FILE* fd, uintptr_t vaddr, size_t size, uintptr_t *file_offset, size_t *max_size)
{


    Elf64_Ehdr ehdr;
    fseek(fd, 0, SEEK_SET);
    if (fread(&ehdr, sizeof(ehdr), 1, fd) != 1)
    {
        fprintf(stderr, "%sFailed to read ELF header%s\n", RED, RESET);
        return 1;
    }

    if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0)
    {
        fprintf(stderr, "%sNot an ELF file%s\n", RED, RESET);
        return 1;
    }

    Elf64_Phdr phdr;
    fseek(fd, ehdr.e_phoff, SEEK_SET);
    for (int i = 0; i < ehdr.e_phnum; i++)
    {
        if (fread(&phdr, sizeof(phdr), 1, fd) != 1)
        {
            fprintf(stderr, "%sFailed to read program header %d%s\n", RED, i, RESET);
            return 1;
        }

        if (phdr.p_type == PT_LOAD && phdr.p_vaddr <= vaddr &&
            vaddr < phdr.p_vaddr + phdr.p_memsz)
        {
            *file_offset = phdr.p_offset + (vaddr - phdr.p_vaddr);
            *max_size = phdr.p_filesz - (vaddr - phdr.p_vaddr);
            if (*max_size < size)
            {
                printf("Adjusted size from %zu to %zu to fit section\n", size, *max_size);
                size = *max_size;
            }
            return 0;
        }
    }

    fprintf(stderr, "%sVirtual address 0x%016lx not found in any loadable segment%s\n", RED, vaddr, RESET);
    return 1;
}


/**
 * parses the binary to get the function name and bounds
 */
int get_function_bounds(const char *binary_path, const char *func_name, uintptr_t *start_addr, size_t *size)
{
    if (elf_version(EV_CURRENT) == EV_NONE)
    {
        fprintf(stderr, "%sELF library initialization failed: %s%s\n", RED, elf_errmsg(-1), RESET);
        return 1;
    }

    int fd = open(binary_path, O_RDONLY);
    if (fd < 0)
    {
        fprintf(stderr, "%sCannot open %s: %s%s\n", RED, binary_path, strerror(errno), RESET);
        return 1;
    }

    Elf *elf = elf_begin(fd, ELF_C_READ, NULL);
    if (!elf)
    {
        fprintf(stderr, "%self_begin failed: %s%s\n", RED, elf_errmsg(-1), RESET);
        close(fd);
        return 1;
    }

    size_t shstrndx;
    if (elf_getshdrstrndx(elf, &shstrndx) != 0)
    {
        fprintf(stderr, "%self_getshdrstrndx failed: %s%s\n", RED, elf_errmsg(-1), RESET);
        elf_end(elf);
        close(fd);
        return 1;
    }

    Elf_Scn *scn = NULL;
    while ((scn = elf_nextscn(elf, scn)) != NULL)
    {
        GElf_Shdr shdr;
        if (!gelf_getshdr(scn, &shdr))
        {
            continue;
        }

        if (shdr.sh_type == SHT_SYMTAB)
        {
            Elf_Data *data = elf_getdata(scn, NULL);
            size_t count = shdr.sh_size / shdr.sh_entsize;

            for (size_t i = 0; i < count; ++i)
            {
                GElf_Sym sym;
                if (!gelf_getsym(data, i, &sym))
                {
                    continue;
                }

                if (GELF_ST_TYPE(sym.st_info) == STT_FUNC)
                {
                    const char *name = elf_strptr(elf, shdr.sh_link, sym.st_name);
                    if (name && strcmp(name, func_name) == 0)
                    {
                        *start_addr = sym.st_value;
                        *size = sym.st_size;
                        elf_end(elf);
                        close(fd);
                        return 0; 
                    }
                }
            }
        }
    }

    fprintf(stderr, "%sFunction '%s' not found in %s%s\n", RED, func_name, binary_path, RESET);
    elf_end(elf);
    close(fd);
    return 1;
}

int disassemble(struct sylvan_inferior *inf, uintptr_t start_addr, uintptr_t end_addr, struct disassembled_instruction **instructions, int *count)
{
    if (!inf || !inf->realpath || !instructions || !count || start_addr >= end_addr)
    {
        fprintf(stderr, "%sInvalid arguments or address range%s\n", RED, RESET);
        return 1;
    }

    FILE *fd = fopen(inf->realpath, "rb");
    if (!fd)
    {
        perror("Failed to open the file");
        return 1;
    }

    size_t size = end_addr - start_addr;
    uintptr_t file_offset;
    size_t max_size;

    if (get_file_offset_from_vaddr(fd, start_addr, size, &file_offset, &max_size) != 0)
    {
        fclose(fd);
        return 1;
    }

    if (size > max_size)
    {
        size = max_size;
    }

    uint8_t *buffer = malloc(size);
    if (!buffer)
    {
        fprintf(stderr, "%sMemory allocation failed%s\n", RED, RESET);
        fclose(fd);
        return 1;
    }

    int success = 0;
    struct disassembled_instruction *head = NULL;
    struct disassembled_instruction *tail = NULL;
    int instr_count = 0;

    if (fseek(fd, file_offset, SEEK_SET) != 0)
    {
        fprintf(stderr, "%sFailed to seek to offset 0x%016lx%s\n", RED, file_offset, RESET);
    }
    else
    {
        size_t bytes_read = fread(buffer, 1, size, fd);
        if (bytes_read != size)
        {
            fprintf(stderr, "%sFailed to read %zu bytes from file (read %zu)%s\n",
                    RED, size, bytes_read, RESET);
        }
        else
        {
            ZyanU64 runtime_address = start_addr;
            ZyanUSize offset = 0;
            ZydisDisassembledInstruction instr;

            while (offset < size &&
                   ZYAN_SUCCESS(ZydisDisassembleIntel(
                       ZYDIS_MACHINE_MODE_LONG_64,
                       runtime_address,
                       buffer + offset,
                       size - offset,
                       &instr)))
            {
                ZyanUSize len = instr.info.length;

                struct disassembled_instruction *node = malloc(sizeof(struct disassembled_instruction));
                if (!node)
                {
                    fprintf(stderr, "%sMemory allocation failed (node)%s\n", RED, RESET);
                    break;
                }

                node->addr = runtime_address;
                node->next = NULL;

                node->opcodes = malloc(len * 3 + 1);
                if (!node->opcodes)
                {
                    free(node);
                    fprintf(stderr, "%sMemory allocation failed (opcodes)%s\n", RED, RESET);
                    break;
                }

                char *p = node->opcodes;
                for (ZyanUSize i = 0; i < len; ++i)
                {
                    sprintf(p, "%02X ", buffer[offset + i]);
                    p += 3;
                }
                *(p - 1) = '\0';

                node->instruction = strdup(instr.text);
                if (!node->instruction)
                {
                    free(node->opcodes);
                    free(node);
                    fprintf(stderr, "%sMemory allocation failed (instruction)%s\n", RED, RESET);
                    break;
                }

                if (!head)
                {
                    head = node;
                    tail = node;
                }
                else
                {
                    tail->next = node;
                    tail = node;
                }

                offset += len;
                runtime_address += len;
                instr_count++;
            }

            if (offset >= size || instr_count > 0)
            {
                success = 1;
            }
        }
    }

    free(buffer);
    fclose(fd);

    if (success)
    {
        *instructions = head;
        *count = instr_count;
        return 0;
    }
    else
    {
        free_instructions(head);
        *instructions = NULL;
        *count = 0;
        return 1;
    }
}

void print_disassembly(struct disassembled_instruction *instructions, int count)
{
    if (!instructions || count == 0)
    {
        printf("%sNo instructions to display%s\n", YELLOW, RESET);
        return;
    }

    struct table_col cols[] = {
        {"Address", 18, TABLE_COL_HEX_LONG},
        {"Opcodes", 34, TABLE_COL_STR},
        {"Instruction", 40, TABLE_COL_STR}};

    struct table_row *rows = NULL, *current = NULL;
    struct disassembled_instruction *inst = instructions;

    while (inst)
    {
        struct table_row *new_row = malloc(sizeof(struct table_row));
        void *row_data = malloc(sizeof(uintptr_t) + 2 * sizeof(char *));
        *(uintptr_t *)row_data = inst->addr;
        *(char **)(row_data + sizeof(uintptr_t)) = strdup(inst->opcodes);
        *(char **)(row_data + sizeof(uintptr_t) + sizeof(char *)) = strdup(inst->instruction);
        new_row->data = row_data;
        new_row->next = NULL;

        if (!rows)
            rows = new_row;
        else
            current->next = new_row;
        current = new_row;

        inst = inst->next;
    }

    print_table("Disassembly", cols, 3, rows, count);

    current = rows;
    while (current)
    {
        struct table_row *next = current->next;
        free((char *)(*(char **)((char *)current->data + sizeof(uintptr_t))));
        free((char *)(*(char **)((char *)current->data + sizeof(uintptr_t) + sizeof(char *))));
        free((void *)current->data);
        free(current);
        current = next;
    }
}
#include <stdio.h>
#include <stdlib.h>
#include <Zydis/Zydis.h>
#include <string.h>

#include "sylvan/inferior.h"
#include "disassemble.h"
#include "ui_utils.h"

int disassemble(struct sylvan_inferior *inf, uintptr_t start_addr, uintptr_t end_addr,
                struct disassembled_instruction **instructions, int *count)
{
    if (!inf || !instructions || !count || start_addr >= end_addr)
    {
        fprintf(stderr, "%sInvalid arguments or address range%s\n", RED, RESET);
        return 1;
    }

    size_t size = end_addr - start_addr;
    uint8_t *buffer = malloc(size);
    if (!buffer)
    {
        fprintf(stderr, "%sMemory allocation failed%s\n", RED, RESET);
        return 1;
    }

    for (size_t i = 0; i < size; i += 8)
    {
        uint64_t data;
        uintptr_t addr = start_addr + i;
        size_t bytes_to_read = (size - i >= 8) ? 8 : (size - i);
        if (sylvan_get_memory(inf, addr, &data) != SYLVANC_OK)
        {
            fprintf(stderr, "%sFailed to read memory at 0x%016lx: %s%s\n",
                    RED, addr, sylvan_get_last_error(), RESET);
            free(buffer);
            return 1;
        }
        memcpy(buffer + i, &data, bytes_to_read);
    }

    ZyanU64 runtime_address = start_addr;
    ZyanUSize offset = 0;
    ZydisDisassembledInstruction instr;

    struct disassembled_instruction *head = NULL;
    struct disassembled_instruction *tail = NULL;
    int instr_count = 0;

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

        node->opcodes = malloc(len * 3 + 1); // +1 for null terminator
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

    free(buffer);
    *instructions = head;
    *count = instr_count;

    return 0;
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
        {"Opcodes", 24, TABLE_COL_STR},
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
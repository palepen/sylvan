#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <stddef.h>
#include <sys/user.h>

#include "register.h"
#include "ui_utils.h"

const struct sylvan_register sylvan_registers_info[] =
    {
#define DEFINE_REGISTER(name, dwarf_id, size, offset, type, format) \
    {name, #name, dwarf_id, size, offset, type, format}
#include "defs/register_info.h"
#undef DEFINE_REGISTER
        {0, NULL, 0, 0, 0, 0, 0}};

        
/**
 * @brief print all the registers
 */
void print_registers(struct user_regs_struct *regs)
{
    struct table_col cols[] = {
        {"REGISTER", 12, TABLE_COL_STR},
        {"VALUE", 20, TABLE_COL_HEX_LONG}};
    int col_count = 2;

    int row_count = 0;
    for (int i = 0; sylvan_registers_info[i].name != NULL; i++)
        row_count++;

    struct table_row *rows = NULL, *current = NULL;
    for (int i = 0; i < row_count; i++)
    {
        uint64_t data;
        memcpy(&data, (uint8_t *)regs + sylvan_registers_info[i].offset, sizeof(uint64_t));

        struct table_row *new_row = malloc(sizeof(struct table_row));
        void *row_data = malloc(sizeof(char *) + sizeof(uint64_t));
        *(const char **)row_data = sylvan_registers_info[i].name;
        *(uint64_t *)(row_data + sizeof(char *)) = data;
        new_row->data = row_data;
        new_row->next = NULL;

        if (!rows)
            rows = new_row;
        else
            current->next = new_row;
        current = new_row;
    }

    print_table("CPU REGISTER MONITOR", cols, col_count, rows, row_count);

    current = rows;
    while (current)
    {
        struct table_row *next = current->next;
        free((void *)current->data);
        free(current);
        current = next;
    }
}

/**
 * @brief get the index of the register with name
 * @return idx if found and -1 if not
 */
int find_register_by_name(char *reg_name)
{
    for (int i = 0; sylvan_registers_info[i].name != NULL; i++)
    {
        if (strcmp(reg_name, sylvan_registers_info[i].name) == 0)
        {
            return i;
        }
    }

    return -1;
}

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
    {0, NULL, 0, 0, 0, 0, 0}
};

void print_top(int width)
{
    printf("%s", GRAY);
    printf("┌");
    for (int i = 0; i < 12; i++)
        printf("─");
    printf("┬");
    for (int i = 0; i < width - 15; i++)
        printf("─");
    printf("┐%s\n", RESET);

}

void print_mid_separator(int width)
{
    printf("%s", GRAY);
    printf("├");
    for (int i = 0; i < 12; i++)
        printf("─");
    printf("┼");
    for (int i = 0; i < width - 15; i++)
        printf("─");
    printf("┤%s\n", RESET);
}


void print_bottom_separator(int width)
{
    printf("%s%s", GRAY, BOLD);
    printf("└");
    for (int i = 0; i < 12; i++)
        printf("─");
    printf("┴");
    for (int i = 0; i < width - 15; i++)
        printf("─");
    printf("┘%s\n", RESET);
}


/**
 * @brief print all the registers
 */
void print_registers(struct user_regs_struct *regs)
{
    int width = 80;
    
    printf("\n %s%sCPU REGISTER MONITOR%s\n", BOLD, CYAN, RESET);
    
    print_top(width);
    printf("%s│%s  %sREGISTER%s  %s│ %sVALUE%s",
        GRAY, RESET, BOLD, RESET, GRAY, BOLD, RESET);
    
    
    printf("\033[%dC", width - 21);
    
    printf("%s│%s\n", GRAY, RESET);  
    
    print_mid_separator(width);
    

    
    for(int i = 0; sylvan_registers_info[i].name != NULL; i++)
    {
        uint64_t data;
        memcpy(&data, (uint8_t *)regs + sylvan_registers_info[i].offset, sizeof(uint64_t));
        
        printf("%s│%s", GRAY, RESET);
        char *name = sylvan_registers_info[i].name;
        int name_len = strlen(name);
        printf(" %s", name);
        for(int i = 0; i < 11 - name_len; i++)
            printf(" ");
        printf("%s│%s 0x%016lx \033[%dC%s│%s\n",GRAY, RESET, data, width - 35, GRAY, RESET);
        
        if(sylvan_registers_info[i + 1].name == NULL)
        {
            print_bottom_separator(width);
        }
        else
        {
            print_mid_separator(width);
        }
    }

}    

/**
 * @brief get the index of the register with name
 * @return idx if found and -1 if not
 */
int find_register_by_name(char *reg_name)
{
    for(int i = 0; sylvan_registers_info[i].name != NULL; i++)
    {
        if(strcmp(reg_name, sylvan_registers_info[i].name) == 0)
        {
            return i;
        }
    }

    return -1;
}


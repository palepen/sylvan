#ifndef DISASSEMBLE_H
#define DISASSEMBLE_H

#include <stdint.h>

struct disassembled_instruction
{
    uintptr_t addr;
    char *opcodes;
    char *instruction;
    struct disassembled_instruction *next;
};

int disassemble(struct sylvan_inferior *inf, uintptr_t start_addr, uintptr_t end_addr, struct disassembled_instruction **instructions, int *count);
void print_disassembly(struct disassembled_instruction *instructions, int count);

#endif
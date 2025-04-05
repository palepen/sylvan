#ifndef REGISTER_H
#define REGISTER_H
#include <stdint.h>
#include <sys/user.h>
#include <stddef.h>

enum sylvan_register_type
{
    SYLVAN_GPR
};

enum sylvan_register_id
{

    #define DEFINE_REGISTER(name, dwarf_id, size, offset, type, format) name
    #include "defs/register_info.h"
    #undef DEFINE_REGISTER
    NUM_REGISTERS
};
enum sylvan_register_format
{
    SYLVAN_REG_FORMAT_UINT, /**< Unsigned integer format */
};

/**
 * @brief Structure to hold metadata about a CPU register.
 */
struct sylvan_register
{
    enum sylvan_register_id id;         /**< Unique identifier for the register */
    const char *name;                   /**< Human-readable name of the register (e.g., "rax") */
    int32_t dwarf_id;                   /**< DWARF register number (used in debugging standards) */
    size_t size;                        /**< Size of the register in bytes (e.g., 8 for 64-bit) */
    size_t offset;                      /**< Offset within a register set */
    enum sylvan_register_type type;     /**< Type of the register (GPR, FPR, etc.) */
    enum sylvan_register_format format; /**< Format of the register's data (uint, float, etc.) */
};

/**
 * @brief constant array of register information.
 */
extern const struct sylvan_register sylvan_registers_info[];

void print_registers(struct user_regs_struct *regs);
int find_register_by_name(char *reg_name);

#endif
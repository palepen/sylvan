#ifndef REGISTER_H
#define REGISTER_H
#include <stdint.h>
#include <sys/user.h>

enum sylvan_register_type
{
    SYLVAN_GPR
};

enum sylvan_register_id
{
    rax = 0,        /**< RAX: 64-bit accumulator */
    rdx,            /**< RDX: 64-bit data register */
    rcx,            /**< RCX: 64-bit counter register */
    rbx,            /**< RBX: 64-bit base register */
    rsi,            /**< RSI: 64-bit source index */
    rdi,            /**< RDI: 64-bit destination index */
    rbp,            /**< RBP: 64-bit base pointer */
    rsp,            /**< RSP: 64-bit stack pointer */
    r8 ,            /**< R8: 64-bit general-purpose register */
    r9 ,            /**< R9: 64-bit general-purpose register */
    r10,            /**< R10: 64-bit general-purpose register */
    r11,            /**< R11: 64-bit general-purpose register */
    r12,            /**< R12: 64-bit general-purpose register */
    r13,            /**< R13: 64-bit general-purpose register */
    r14,            /**< R14: 64-bit general-purpose register */
    r15,            /**< R15: 64-bit general-purpose register */
    rip,            /**< RIP: 64-bit instruction pointer */
    eflags = 49,    /**< EFLAGS: 64-bit flags register */
    es,             /**< ES: Extra segment register */
    cs,             /**< CS: Code segment register */
    ss,             /**< SS: Stack segment register */
    ds,             /**< DS: Data segment register */
    fs,             /**< FS: Extra segment register */
    gs,             /**< GS: Extra segment register */
    orig_rax = -1,  /**< ORIG_RAX: Original RAX value before syscall */
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

#endif
#ifndef DEFINE_REGISTER
#error "This file is intended for textual inclusion with the DEFINE_REGISTER macro defined"
#endif


#define GET_OFFSET(reg) (offsetof(struct user_regs_struct, reg))

#define DEFINE_GPR_64(name, dwarf_id) \
    DEFINE_REGISTER(name, dwarf_id, 8, GET_OFFSET(name), SYLVAN_GPR, SYLVAN_REG_FORMAT_UINT)

/* 64-bit GPRs */
DEFINE_GPR_64(rax, 0),    /**< RAX: 64-bit accumulator */
DEFINE_GPR_64(rdx, 1),    /**< RDX: 64-bit data register */
DEFINE_GPR_64(rcx, 2),    /**< RCX: 64-bit counter register */
DEFINE_GPR_64(rbx, 3),    /**< RBX: 64-bit base register */
DEFINE_GPR_64(rsi, 4),    /**< RSI: 64-bit source index */
DEFINE_GPR_64(rdi, 5),    /**< RDI: 64-bit destination index */
DEFINE_GPR_64(rbp, 6),    /**< RBP: 64-bit base pointer */
DEFINE_GPR_64(rsp, 7),    /**< RSP: 64-bit stack pointer */
DEFINE_GPR_64(r8, 8),     /**< R8: 64-bit general-purpose register */
DEFINE_GPR_64(r9, 9),     /**< R9: 64-bit general-purpose register */
DEFINE_GPR_64(r10, 10),   /**< R10: 64-bit general-purpose register */
DEFINE_GPR_64(r11, 11),   /**< R11: 64-bit general-purpose register */
DEFINE_GPR_64(r12, 12),   /**< R12: 64-bit general-purpose register */
DEFINE_GPR_64(r13, 13),   /**< R13: 64-bit general-purpose register */
DEFINE_GPR_64(r14, 14),   /**< R14: 64-bit general-purpose register */
DEFINE_GPR_64(r15, 15),   /**< R15: 64-bit general-purpose register */
DEFINE_GPR_64(rip, 16),   /**< RIP: 64-bit instruction pointer */
DEFINE_GPR_64(eflags, 49),/**< EFLAGS: 64-bit flags register */
DEFINE_GPR_64(cs, 51),    /**< CS: Code segment register */
DEFINE_GPR_64(fs, 54),    /**< FS: Extra segment register */
DEFINE_GPR_64(gs, 55),    /**< GS: Extra segment register */
DEFINE_GPR_64(ss, 52),    /**< SS: Stack segment register */
DEFINE_GPR_64(ds, 53),    /**< DS: Data segment register */
DEFINE_GPR_64(es, 50),    /**< ES: Extra segment register */

DEFINE_GPR_64(orig_rax, -1), /**< ORIG_RAX: Original RAX value before syscall */

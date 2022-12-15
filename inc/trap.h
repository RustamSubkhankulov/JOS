#ifndef JOS_INC_TRAP_H
#define JOS_INC_TRAP_H

/* Trap numbers
 * These are processor defined: */
#define T_DIVIDE 0       /* divide error */
#define T_DEBUG  1       /* debug exception */
#define T_NMI    2       /* non-maskable interrupt */
#define T_BRKPT  3       /* breakpoint */
#define T_OFLOW  4       /* overflow */
#define T_BOUND  5       /* bounds check */
#define T_ILLOP  6       /* illegal opcode */
#define T_DEVICE 7       /* device not available */
#define T_DBLFLT 8       /* double fault */
/* #define T_COPROC 9 */ /* reserved (not generated by recent processors) */
#define T_TSS   10       /* invalid task switch segment */
#define T_SEGNP 11       /* segment not present */
#define T_STACK 12       /* stack exception */
#define T_GPFLT 13       /* general protection fault */
#define T_PGFLT 14       /* page fault */
/* #define T_RES 15 */   /* reserved */
#define T_FPERR   16     /* floating point error */
#define T_ALIGN   17     /* aligment check */
#define T_MCHK    18     /* machine check */
#define T_SIMDERR 19     /* SIMD floating point error */

/* These are arbitrarily chosen, but with care not to overlap
 * processor defined exceptions or interrupt vectors.*/
#define T_SYSCALL 48  /* system call */
#define T_DEFAULT 500 /* catchall */

#define IRQ_OFFSET 32 /* IRQ 0 corresponds to int IRQ_OFFSET */

/* Hardware IRQ numbers. We receive these as (IRQ_OFFSET+IRQ_WHATEVER) */
#define IRQ_TIMER    0
#define IRQ_KBD      1
#define IRQ_SERIAL   4
#define IRQ_SPURIOUS 7
#define IRQ_CLOCK    8
#define IRQ_NIC      11
#define IRQ_IDE      14
#define IRQ_ERROR    19

#define UTRAP_RSP 152
#define UTRAP_RIP 136

#ifndef __ASSEMBLER__

#include <inc/types.h>

struct PushRegs {
    /* Registers as pushed by pusha */
    uint64_t reg_r15;
    uint64_t reg_r14;
    uint64_t reg_r13;
    uint64_t reg_r12;
    uint64_t reg_r11;
    uint64_t reg_r10;
    uint64_t reg_r9;
    uint64_t reg_r8;
    uint64_t reg_rsi;
    uint64_t reg_rdi;
    uint64_t reg_rbp;
    uint64_t reg_rdx;
    uint64_t reg_rcx;
    uint64_t reg_rbx;
    uint64_t reg_rax;
} __attribute__((packed));

struct Trapframe {
    struct PushRegs tf_regs;
    uint16_t tf_es;
    uint16_t tf_padding1;
    uint32_t tf_padding2;
    uint16_t tf_ds;
    uint16_t tf_padding3;
    uint32_t tf_padding4;
    uint64_t tf_trapno;
    /* Below here defined by x86 hardware */
    uint64_t tf_err;
    uintptr_t tf_rip;
    uint16_t tf_cs;
    uint16_t tf_padding5;
    uint32_t tf_padding6;
    uint64_t tf_rflags;
    /* Below here only when crossing rings, such as from user to kernel */
    uintptr_t tf_rsp;
    uint16_t tf_ss;
    uint16_t tf_padding7;
    uint32_t tf_padding8;
} __attribute__((packed));

struct UTrapframe {
    /* Information about the fault */
    uint64_t utf_fault_va; /* va for T_PGFLT, 0 otherwise */
    uint64_t utf_err;
    /* Trap-time return state */
    struct PushRegs utf_regs;
    uintptr_t utf_rip;
    uint64_t utf_rflags;
    /* The trap-time stack to return to */
    uintptr_t utf_rsp;
} __attribute__((packed));

#endif /* !__ASSEMBLER__ */

#endif /* !JOS_INC_TRAP_H */

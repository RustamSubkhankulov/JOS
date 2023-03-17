/* Main public header file for our user-land support library,
 * whose code lives in the lib directory.
 * This library is roughly our OS's version of a standard C library,
 * and is intended to be linked into all user-mode applications
 * (NOT the kernel or boot loader). */

#ifndef JOS_INC_LIB_H
#define JOS_INC_LIB_H 1

#include <inc/types.h>
#include <inc/stdio.h>
#include <inc/stdarg.h>
#include <inc/string.h>
#include <inc/error.h>
#include <inc/assert.h>
#include <inc/env.h>
#include <inc/memlayout.h>
#include <inc/syscall.h>
#include <inc/trap.h>

#ifdef SANITIZE_USER_SHADOW_BASE
/* asan unpoison routine used for whitelisting regions. */
void platform_asan_unpoison(void *, size_t);
void platform_asan_poison(void *, size_t);
/* non-sanitized memcpy and memset allow us to access "invalid" areas for extra poisoning. */
void *__nosan_memset(void *, int, size_t);
void *__nosan_memcpy(void *, const void *src, size_t);
#endif

#define USED(x) (void)(x)

/* main user program */
void umain(int argc, char **argv);

/* libmain.c or entry.S */
extern const char *binaryname;
extern const volatile struct Env *thisenv;
extern const volatile struct Env envs[NENV];

/* exit.c */
void exit(void);

/* pgfault.c */
typedef bool(pf_handler_t)(struct UTrapframe *utf);
int add_pgfault_handler(pf_handler_t handler);
void remove_pgfault_handler(pf_handler_t handler);

/* readline.c */
char *readline(const char *buf);

/* syscall.c */
#define CURENVID 0

/* sys_alloc_region() specific flags */
#define ALLOC_ZERO 0x100000 /* Allocate memory filled with 0x00 */
#define ALLOC_ONE  0x200000 /* Allocate memory filled with 0xFF */

/* Memory protection flags & attributes
 * NOTE These should be in-sync with kern/pmap.h
 * TODO Create dedicated header for them */
#define PROT_X       0x1 /* Executable */
#define PROT_W       0x2 /* Writable */
#define PROT_R       0x4 /* Readable (mostly ignored) */
#define PROT_RW      (PROT_R | PROT_W)
#define PROT_WC      0x8 /* Write-combining */
#define PROT_CD      0x18 /* Cache disable */
#define PROT_SHARE   0x40 /* Shared copy flag */
#define PROT_LAZY    0x80 /* Copy-on-Write flag */
#define PROT_COMBINE 0x100 /* Combine old and new priviliges */
#define PROT_AVAIL   0xA00 /* Free-to-use flags, available for applications */
/* (mapped directly to page table unused flags) */
#define PROT_ALL 0x05F /* NOTE This definition differs from kernel definition */

void sys_cputs(const char *string, size_t len);
int sys_cgetc(void);
envid_t sys_getenvid(void);
int sys_env_destroy(envid_t);
void sys_yield(void);
int sys_region_refs(void *va, size_t size);
int sys_region_refs2(void *va, size_t size, void *va2, size_t size2);
static envid_t sys_exofork(void);
int sys_env_set_status(envid_t env, int status);
int sys_env_set_trapframe(envid_t env, struct Trapframe *tf);
int sys_env_set_pgfault_upcall(envid_t env, void *upcall);
int sys_alloc_region(envid_t env, void *pg, size_t size, int perm);
int sys_map_region(envid_t src_env, void *src_pg,
                   envid_t dst_env, void *dst_pg, size_t size, int perm);
int sys_unmap_region(envid_t env, void *pg, size_t size);
int sys_ipc_try_send(envid_t to_env, uint64_t value, void *pg, size_t size, int perm);
int sys_ipc_recv(void *rcv_pg, size_t size);

/* This must be inlined. Exercise for reader: why? */
static inline envid_t __attribute__((always_inline))
sys_exofork(void) {
    envid_t ret;
    asm volatile("int %2"
                 : "=a"(ret)
                 : "a"(SYS_exofork), "i"(T_SYSCALL));
    return ret;
}

/* ipc.c */
void ipc_send(envid_t to_env, uint32_t value, void *pg, size_t size, int perm);
int32_t ipc_recv(envid_t *from_env_store, void *pg, size_t *psize, int *perm_store);
envid_t ipc_find_env(enum EnvType type);

/* fork.c */
envid_t fork(void);
envid_t sfork(void);

/* uvpt.c */
int foreach_shared_region(int (*fun)(void *start, void *end, void *arg), void *arg);
pte_t get_uvpt_entry(void *addr);
int get_prot(void *va);
bool is_page_dirty(void *va);
bool is_page_present(void *va);

#ifdef JOS_PROG
extern void (*volatile sys_exit)(void);
extern void (*volatile sys_yield)(void);
#endif

#ifndef debug
#define debug 0
#endif

#endif /* !JOS_INC_LIB_H */

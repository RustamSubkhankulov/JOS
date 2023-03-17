/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/elf.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/monitor.h>
#include <kern/sched.h>
#include <kern/kdebug.h>
#include <kern/macro.h>
#include <kern/pmap.h>
#include <kern/traceopt.h>

/* Currently active environment */
struct Env *curenv = NULL;

#ifdef CONFIG_KSPACE
/* All environments */
struct Env env_array[NENV];
struct Env *envs = env_array;
#else
/* All environments */
struct Env *envs = NULL;
#endif

/* Free environment list
 * (linked by Env->env_link) */
static struct Env *env_free_list;


/* NOTE: Should be at least LOGNENV */
#define ENVGENSHIFT 12

/* Converts an envid to an env pointer.
 * If checkperm is set, the specified environment must be either the
 * current environment or an immediate child of the current environment.
 *
 * RETURNS
 *     0 on success, -E_BAD_ENV on error.
 *   On success, sets *env_store to the environment.
 *   On error, sets *env_store to NULL. */
int
envid2env(envid_t envid, struct Env **env_store, bool need_check_perm) {
    struct Env *env;

    /* If envid is zero, return the current environment. */
    if (!envid) {
        *env_store = curenv;
        return 0;
    }

    /* Look up the Env structure via the index part of the envid,
     * then check the env_id field in that struct Env
     * to ensure that the envid is not stale
     * (i.e., does not refer to a _previous_ environment
     * that used the same slot in the envs[] array). */
    env = &envs[ENVX(envid)];
    if (env->env_status == ENV_FREE || env->env_id != envid) {
        *env_store = NULL;
        return -E_BAD_ENV;
    }

    /* Check that the calling environment has legitimate permission
     * to manipulate the specified environment.
     * If checkperm is set, the specified environment
     * must be either the current environment
     * or an immediate child of the current environment. */
    if (need_check_perm && env != curenv && env->env_parent_id != curenv->env_id) {
        *env_store = NULL;
        return -E_BAD_ENV;
    }

    *env_store = env;
    return 0;
}

/* Mark all environments in 'envs' as free, set their env_ids to 0,
 * and insert them into the env_free_list.
 * Make sure the environments are in the free list in the same order
 * they are in the envs array (i.e., so that the first call to
 * env_alloc() returns envs[0]).
 */
void
env_init(void) {
    /* kzalloc_region only works with current_space != NULL */

    /* Allocate envs array with kzalloc_region
     * (don't forget about rounding) */
    // LAB 8: Your code here+

    size_t envs_mem_size = ROUNDUP(sizeof(struct Env) * NENV, PAGE_SIZE);
    void*  envs_mem = kzalloc_region(envs_mem_size);
    assert(envs_mem != NULL);

    /* Map envs to UENVS read-only,
     * but user-accessible (with PROT_USER_ set) */
    // LAB 8: Your code here+

    int res = map_region(&kspace, UENVS, &kspace, (uintptr_t) envs_mem, envs_mem_size, PROT_R | PROT_USER_);
    if (res < 0) panic("env_init: %i\n", res);

    envs = (struct Env*) envs_mem;

    /* Set up envs array */

    // LAB 3: Your code here

    env_free_list = envs;

    uint32_t last_env_iter = NENV - 1;

    for (uint32_t iter = 0; iter < NENV; iter++)
    {
        struct Env* cur_env = &envs[iter];

        cur_env->env_id        = 0;
        cur_env->env_parent_id = 0;

        cur_env->env_type   = ENV_TYPE_IDLE;
        cur_env->env_status = ENV_FREE;

        cur_env->binary = NULL;

        if (iter != last_env_iter)
        {
            cur_env->env_link = cur_env + 1;
        }
        else 
        {
            cur_env->env_link = NULL;
        }
    }

    return;
}

/* Allocates and initializes a new environment.
 * On success, the new environment is stored in *newenv_store.
 *
 * Returns
 *     0 on success, < 0 on failure.
 * Errors
 *    -E_NO_FREE_ENV if all NENVS environments are allocated
 *    -E_NO_MEM on memory exhaustion
 */
int
env_alloc(struct Env **newenv_store, envid_t parent_id, enum EnvType type) {

    struct Env *env;
    if (!(env = env_free_list))
        return -E_NO_FREE_ENV;

    /* Allocate and set up the page directory for this environment. */
    int res = init_address_space(&env->address_space);
    if (res < 0) return res;

    /* Generate an env_id for this environment */
    int32_t generation = (env->env_id + (1 << ENVGENSHIFT)) & ~(NENV - 1);
    /* Don't create a negative env_id */
    if (generation <= 0) generation = 1 << ENVGENSHIFT;
    env->env_id = generation | (env - envs);

    /* Set the basic status variables */
    env->env_parent_id = parent_id;
#ifdef CONFIG_KSPACE
    env->env_type = ENV_TYPE_KERNEL;
#else
    env->env_type = type;
#endif
    env->env_status = ENV_RUNNABLE;
    env->env_runs = 0;

    /* Clear out all the saved register state,
     * to prevent the register values
     * of a prior environment inhabiting this Env structure
     * from "leaking" into our new environment */
    memset(&env->env_tf, 0, sizeof(env->env_tf));

    /* Set up appropriate initial values for the segment registers.
     * GD_UD is the user data (KD - kernel data) segment selector in the GDT, and
     * GD_UT is the user text (KT - kernel text) segment selector (see inc/memlayout.h).
     * The low 2 bits of each segment register contains the
     * Requestor Privilege Level (RPL); 3 means user mode, 0 - kernel mode.  When
     * we switch privilege levels, the hardware does various
     * checks involving the RPL and the Descriptor Privilege Level
     * (DPL) stored in the descriptors themselves */

#ifdef CONFIG_KSPACE
    env->env_tf.tf_ds = GD_KD;
    env->env_tf.tf_es = GD_KD;
    env->env_tf.tf_ss = GD_KD;
    env->env_tf.tf_cs = GD_KT;

    // LAB 3: Your code here:
    static uintptr_t stack_top = 0x2000000;

    env->env_tf.tf_rsp = stack_top;
    stack_top += PROG_STACK_SIZE;

#else
    env->env_tf.tf_ds = GD_UD | 3;
    env->env_tf.tf_es = GD_UD | 3;
    env->env_tf.tf_ss = GD_UD | 3;
    env->env_tf.tf_cs = GD_UT | 3;
    env->env_tf.tf_rsp = USER_STACK_TOP;
#endif

    /* For now init trapframe with IF set */
    // env->env_tf.tf_rflags = FL_IF;

    /* Commit the allocation */
    env_free_list = env->env_link;
    *newenv_store = env;

    if (trace_envs) cprintf("[%08x] new env %08x\n", curenv ? curenv->env_id : 0, env->env_id);
    return 0;
}

/* Pass the original ELF image to binary/size and bind all the symbols within
 * its loaded address space specified by image_start/image_end.
 * Make sure you understand why you need to check that each binding
 * must be performed within the image_start/image_end range.
 */
static int
bind_functions(struct Env *env, uint8_t *binary, size_t size, struct Image_bounds* bounds, 
                                                                         const int bounds_num) {
    // LAB 3: Your code here:

    /* NOTE: find_function from kdebug.c should be used */

    const struct Elf* elf_header   = (const struct Elf*) binary;
    
    const struct Secthdr* sec_headers      = (const struct Secthdr*) (binary + elf_header->e_shoff);
    const        uint16_t sec_headers_num  = (const        uint16_t) elf_header->e_shnum;

    // check whether whole section header table is within image
    if ((void*) (sec_headers) < (void*) binary || 
        (void*) (sec_headers + sec_headers_num) > (void*) (binary + size))
        return -E_INVALID_EXE;

    int16_t string_tab_ind = -1;

    const struct Secthdr* shstr_header = sec_headers + elf_header->e_shstrndx;
    
    uint64_t shstr_offs = shstr_header->sh_offset; 
    uint64_t shstr_size = shstr_header->sh_size;

    // check whether shstr section is within image 
    if (shstr_offs > size 
     || shstr_offs + shstr_size > size)
        return -E_INVALID_EXE;

    const char* shstr = (const char*) (binary + shstr_offs);

    for (uint16_t sec_header_iter = 0; sec_header_iter < sec_headers_num; sec_header_iter++)
    {
        const struct Secthdr* cur_sec_header = sec_headers + sec_header_iter;
    
        uint32_t sh_name = cur_sec_header->sh_name;

        // check whether sh_name is located within
        // if ((void*) (shstr + sh_name) > (void*) (binary + shstr_offs + shstr_size - 8))
            // return -E_INVALID_EXE;
        
        const char* null_check_ptr = shstr + sh_name;
        const char* shstr_end      = shstr + shstr_size;

        bool null_occured = false;

        while (null_check_ptr < shstr_end)
        {
            if (*null_check_ptr == '\0')
            {
                null_occured = true;
                break;
            }

            null_check_ptr++;
        }

        if (!null_occured)
            return -E_INVALID_EXE;

        if (strncmp(".strtab", shstr + sh_name, 8) == 0)
            string_tab_ind = sec_header_iter;
    }

    if (string_tab_ind == -1)
        return -E_INVALID_EXE;

    const struct Secthdr* string_tab_hdr = sec_headers + string_tab_ind;
    const char*           string_tab     = (const char*) (binary + string_tab_hdr->sh_offset);

    if (string_tab_hdr->sh_offset > size 
     || string_tab_hdr->sh_offset + string_tab_hdr->sh_size > size)
        return -E_INVALID_EXE;

    for (uint16_t sec_header_iter = 0; sec_header_iter < sec_headers_num; sec_header_iter++)
    {
        const struct Secthdr* cur_sec_header = sec_headers + sec_header_iter;

        if (cur_sec_header->sh_type != ELF_SHT_SYMTAB)
            continue;

        if (cur_sec_header->sh_size == 0)
            continue;

        const struct Elf64_Sym* sym_tab = (const struct Elf64_Sym*) (binary + cur_sec_header->sh_offset);
        size_t                  sym_num = (size_t) (cur_sec_header->sh_size / sizeof(struct Elf64_Sym));

        if (cur_sec_header->sh_offset > size
         || cur_sec_header->sh_offset + cur_sec_header->sh_size > size)
            return -E_INVALID_EXE;

        for (size_t sym_iter = 0; sym_iter < sym_num; sym_iter++)
        {
            const struct Elf64_Sym* cur_sym = sym_tab + sym_iter;
            uint8_t st_info = cur_sym->st_info;

            if ((ELF64_ST_TYPE(st_info) != STT_OBJECT) 
             || (ELF64_ST_BIND(st_info) != STB_GLOBAL))
                continue;

            if (cur_sym->st_name == 0)
                continue;

            const char* sym_name  = (const char*) (string_tab + cur_sym->st_name);

            const char* null_check_ptr = sym_name;
            const char* string_tab_end = string_tab + string_tab_hdr->sh_size;

            bool null_occured = false;

            while (null_check_ptr < string_tab_end)
            {
                if (*null_check_ptr == '\0')
                {
                    null_occured = true;
                    break;
                }

                null_check_ptr++;
            }

            if (!null_occured)
                return -E_INVALID_EXE;

            uintptr_t   sym_value = (uintptr_t) cur_sym->st_value;

            bool is_correct = false;

            for (unsigned iter = 0; iter < bounds_num; iter++)
            {
                if ((sym_value >= bounds[iter].start) && (sym_value <= bounds[iter].end))
                {    
                    is_correct = true;
                    break;
                }
            }

            if (is_correct == false)
                // panic("bind_functions: sym_value is outside of image: sym_name = %s sym_value = %p \n",
                //                                                       sym_name, (void*) sym_value);
                continue;

            if (*(uintptr_t*) sym_value != 0)
                continue;

            uintptr_t offset = find_function(sym_name);
            if (offset == 0)
                continue;

            *(uintptr_t*) sym_value = offset;       
        }
    }

    return 0;
}

/* Set up the initial program binary, stack, and processor flags
 * for a user process.
 * This function is ONLY called during kernel initialization,
 * before running the first environment.
 *
 * This function loads all loadable segments from the ELF binary image
 * into the environment's user memory, starting at the appropriate
 * virtual addresses indicated in the ELF program header.
 * At the same time it clears to zero any portions of these segments
 * that are marked in the program header as being mapped
 * but not actually present in the ELF file - i.e., the program's bss section.
 *
 * All this is very similar to what our boot loader does, except the boot
 * loader also needs to read the code from disk.  Take a look at
 * LoaderPkg/Loader/Bootloader.c to get ideas.
 *
 * Finally, this function maps one page for the program's initial stack.
 *
 * load_icode returns -E_INVALID_EXE if it encounters problems.
 *  - How might load_icode fail?  What might be wrong with the given input?
 *
 * Hints:
 *   Load each program segment into memory
 *   at the address specified in the ELF section header.
 *   You should only load segments with ph->p_type == ELF_PROG_LOAD.
 *   Each segment's address can be found in ph->p_va
 *   and its size in memory can be found in ph->p_memsz.
 *   The ph->p_filesz bytes from the ELF binary, starting at
 *   'binary + ph->p_offset', should be copied to address
 *   ph->p_va.  Any remaining memory bytes should be cleared to zero.
 *   (The ELF header should have ph->p_filesz <= ph->p_memsz.)
 *   Use functions from the previous labs to allocate and map pages.
 *
 *   All page protection bits should be user read/write for now.
 *   ELF segments are not necessarily page-aligned, but you can
 *   assume for this function that no two segments will touch
 *   the same page.
 *
 *   You may find a function like map_region useful.
 *
 *   Loading the segments is much simpler if you can move data
 *   directly into the virtual addresses stored in the ELF binary.
 *   So which page directory should be in force during
 *   this function?
 *
 *   You must also do something with the program's entry point,
 *   to make sure that the environment starts executing there.
 *   What?  (See env_run() and env_pop_tf() below.) */

static int
load_icode(struct Env *env, uint8_t *binary, size_t size) {
    // LAB 3: Your code here

    struct Image_bounds bounds[Loaded_segments_num];

    const struct Elf* elf_header = (const struct Elf*) binary;

    if (elf_header->e_magic != ELF_MAGIC)
        return -E_INVALID_EXE;

    if (elf_header->e_shentsize != sizeof(struct Secthdr))
        return -E_INVALID_EXE;

    if (elf_header->e_phentsize != sizeof(struct Proghdr))
        return -E_INVALID_EXE;
        
    if (elf_header->e_shstrndx >= elf_header->e_shnum)
        return -E_INVALID_EXE;

    const uint64_t phnum = elf_header->e_phnum;
    const uint64_t phoff = elf_header->e_phoff;

    const struct Proghdr* ph_table = (const struct Proghdr*) (binary + phoff);

    for (uint64_t iter = 0; iter < phnum; iter++)
    {
        const struct Proghdr* cur_ph = ph_table + iter;

        if (cur_ph->p_type != ELF_PROG_LOAD)
            continue;

        cprintf("ph#%ld va:0x%lx pa:0x%lx flags:0x%x \n", iter, cur_ph->p_va, cur_ph->p_pa, cur_ph->p_flags);

        uintptr_t p_va   = cur_ph->p_va;
        uint64_t  filesz = cur_ph->p_filesz;
        uint64_t  memsz  = cur_ph->p_memsz;

        if (iter >= Loaded_segments_num)
            panic("Number of loading segments is more than expected \n");

        bounds[iter].start = p_va;
        bounds[iter].end   = p_va + memsz;
        bounds[iter].size  = memsz;

        int res = map_region(&kspace, p_va, NULL, 0, memsz, PROT_RWX | ALLOC_ZERO);
        if (res < 0) panic("map prog to kspace: %i \n", res);

        memcpy((void*) p_va, binary + cur_ph->p_offset, (size_t) filesz);

        uint32_t prog_flags = cur_ph->p_flags; 
        cprintf("prog_flags: 0x%x \n", prog_flags);

        res = map_region(&env->address_space, p_va, &kspace, p_va, memsz, prog_flags | PROT_USER_);
        if (res < 0) panic("map prog to env->address_space: %i \n", res);

        unmap_region(&kspace, p_va, memsz);
    }

    env->binary = binary;
    env->env_tf.tf_rip = (uintptr_t) elf_header->e_entry;

    // int err = bind_functions(env, binary, size, bounds, Loaded_segments_num);
    // if (err < 0)
    //     panic("bind_functions: %i", err);        
    // for (unsigned iter = 0; iter < phnum; iter++)
    //     unmap_region(&kspace, bounds[iter].start, bounds[iter].size);

    // LAB 8: Your code here +

    int res = map_region(&env->address_space, USER_STACK_TOP - PAGE_SIZE, NULL, 0, PAGE_SIZE, PROT_R | PROT_W | PROT_USER_ | ALLOC_ZERO);
    if (res < 0) panic("load_icode: %i \n", res);

    return 0;
}

/* Allocates a new env with env_alloc, loads the named elf
 * binary into it with load_icode, and sets its env_type.
 * This function is ONLY called during kernel initialization,
 * before running the first user-mode environment.
 * The new env's parent ID is set to 0.
 */
void
env_create(uint8_t *binary, size_t size, enum EnvType type) {
    // LAB 8: Your code here
    // LAB 3: Your code here

    struct Env* new_env = NULL;
    
    int err = env_alloc(&new_env, 0, type);
    if (err < 0) panic("env_alloc: %i", err);

    err = load_icode(new_env, binary, size);
    if (err < 0) panic("load_icode: %i", err);
    
    new_env->env_parent_id = 0;

    return;
}


/* Frees env and all memory it uses */
void
env_free(struct Env *env) {

    /* Note the environment's demise. */
    if (trace_envs) cprintf("[%08x] free env %08x\n", curenv ? curenv->env_id : 0, env->env_id);

#ifndef CONFIG_KSPACE
    /* If freeing the current environment, switch to kern_pgdir
     * before freeing the page directory, just in case the page
     * gets reused. */
    if (&env->address_space == current_space)
        switch_address_space(&kspace);

    static_assert(MAX_USER_ADDRESS % HUGE_PAGE_SIZE == 0, "Misaligned MAX_USER_ADDRESS");
    release_address_space(&env->address_space);
#endif

    /* Return the environment to the free list */
    env->env_status = ENV_FREE;
    env->env_link = env_free_list;
    env_free_list = env;
}

/* Frees environment env
 *
 * If env was the current one, then runs a new environment
 * (and does not return to the caller)
 */
void
env_destroy(struct Env *env) {
    /* If env is currently running on other CPUs, we change its state to
     * ENV_DYING. A zombie environment will be freed the next time
     * it traps to the kernel. */

    // LAB 3: Your code here

    env_free(env);

    if (env == curenv)
        sched_yield();
    // LAB 8: Your code here (set in_page_fault = 0)

    in_page_fault = 0;
}

#ifdef CONFIG_KSPACE
void
csys_exit(void) {
    if (!curenv) panic("curenv = NULL");
    env_destroy(curenv);
}

void
csys_yield(struct Trapframe *tf) {
    memcpy(&curenv->env_tf, tf, sizeof(struct Trapframe));
    sched_yield();
}
#endif

/* Restores the register values in the Trapframe with the 'ret' instruction.
 * This exits the kernel and starts executing some environment's code.
 *
 * This function does not return.
 */

_Noreturn void
env_pop_tf(struct Trapframe *tf) {
    asm volatile(
            "movq %0, %%rsp\n"
            "movq 0(%%rsp), %%r15\n"
            "movq 8(%%rsp), %%r14\n"
            "movq 16(%%rsp), %%r13\n"
            "movq 24(%%rsp), %%r12\n"
            "movq 32(%%rsp), %%r11\n"
            "movq 40(%%rsp), %%r10\n"
            "movq 48(%%rsp), %%r9\n"
            "movq 56(%%rsp), %%r8\n"
            "movq 64(%%rsp), %%rsi\n"
            "movq 72(%%rsp), %%rdi\n"
            "movq 80(%%rsp), %%rbp\n"
            "movq 88(%%rsp), %%rdx\n"
            "movq 96(%%rsp), %%rcx\n"
            "movq 104(%%rsp), %%rbx\n"
            "movq 112(%%rsp), %%rax\n"
            "movw 120(%%rsp), %%es\n"
            "movw 128(%%rsp), %%ds\n"
            "addq $152,%%rsp\n" /* skip tf_trapno and tf_errcode */
            "iretq" ::"g"(tf)
            : "memory");

    /* Mostly to placate the compiler */
    panic("Reached unrecheble\n");
}

/* Context switch from curenv to env.
 * This function does not return.
 *
 * Step 1: If this is a context switch (a new environment is running):
 *       1. Set the current environment (if any) back to
 *          ENV_RUNNABLE if it is ENV_RUNNING (think about
 *          what other states it can be in),
 *       2. Set 'curenv' to the new environment,
 *       3. Set its status to ENV_RUNNING,
 *       4. Update its 'env_runs' counter,
 *       5. Use switch_address_space() to switch to its address space.
 * Step 2: Use env_pop_tf() to restore the environment's
 *       registers and starting execution of process.

 * Hints:
 *    If this is the first call to env_run, curenv is NULL.
 *
 *    This function loads the new environment's state from
 *    env->env_tf.  Go back through the code you wrote above
 *    and make sure you have set the relevant parts of
 *    env->env_tf to sensible values.
 */
_Noreturn void
env_run(struct Env *env) {
    assert(env);

    if (trace_envs_more) {
        const char *state[] = {"FREE", "DYING", "RUNNABLE", "RUNNING", "NOT_RUNNABLE"};
        if (curenv) cprintf("[%08X] env stopped: %s\n", curenv->env_id, state[curenv->env_status]);
        cprintf("[%08X] env started: %s\n", env->env_id, state[env->env_status]);
    }

    // LAB 3: Your code here
    // LAB 8: Your code here+

    if (curenv != env)
    {
        if (curenv != NULL && curenv->env_status == ENV_RUNNING)
            curenv->env_status =  ENV_RUNNABLE;

        curenv = env;
        curenv->env_runs++;
        curenv->env_status = ENV_RUNNING;
        switch_address_space(&curenv->address_space);
    }

    env_pop_tf(&curenv->env_tf);
}

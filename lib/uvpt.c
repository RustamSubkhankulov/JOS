/* User virtual page table helpers */

#include <inc/lib.h>
#include <inc/mmu.h>

extern volatile pte_t uvpt[];     /* VA of "virtual page table" */
extern volatile pde_t uvpd[];     /* VA of current page directory */
extern volatile pdpe_t uvpdp[];   /* VA of current page directory pointer */
extern volatile pml4e_t uvpml4[]; /* VA of current page map level 4 */

pte_t
get_uvpt_entry(void *va) {
    if (!(uvpml4[VPML4(va)] & PTE_P)) return uvpml4[VPML4(va)];
    if (!(uvpdp[VPDP(va)] & PTE_P) || (uvpdp[VPDP(va)] & PTE_PS)) return uvpdp[VPDP(va)];
    if (!(uvpd[VPD(va)] & PTE_P) || (uvpd[VPD(va)] & PTE_PS)) return uvpd[VPD(va)];
    return uvpt[VPT(va)];
}

int
get_prot(void *va) {
    pte_t pte = get_uvpt_entry(va);
    int prot = pte & PTE_AVAIL & ~PTE_SHARE;
    if (pte & PTE_P) prot |= PROT_R;
    if (pte & PTE_W) prot |= PROT_W;
    if (!(pte & PTE_NX)) prot |= PROT_X;
    if (pte & PTE_SHARE) prot |= PROT_SHARE;
    return prot;
}

bool
is_page_dirty(void *va) {
    pte_t pte = get_uvpt_entry(va);
    return pte & PTE_D;
}

bool
is_page_present(void *va) {
    return get_uvpt_entry(va) & PTE_P;
}

bool 
is_page_share(void* va) {
    return get_uvpt_entry(va) & PTE_SHARE;
}

int
foreach_shared_region(int (*fun)(void *start, void *end, void *arg), void *arg) {
    /* Calls fun() for every shared region */
    // LAB 11: Your code here

    uintptr_t start = 0;

    while (start < MAX_USER_ADDRESS) 
    {
        pte_t pte;
        uintptr_t size;

        if (!(uvpml4[VPML4(start)] & PTE_P)) 
        {
            pte = uvpml4[VPML4(start)];
            size = 1ULL << PML4_SHIFT;
        } 
        else if (!(uvpdp[VPDP(start)] & PTE_P) || (uvpdp[VPDP(start)] & PTE_PS)) 
        {
            pte = uvpdp[VPDP(start)];
            size = 1ULL << PDP_SHIFT;
        } 
        else if (!(uvpd[VPD(start)] & PTE_P) || (uvpd[VPD(start)] & PTE_PS)) 
        {
            pte = uvpd[VPD(start)];
            size = 1ULL << PD_SHIFT;
        } 
        else 
        {
            pte = uvpt[VPT(start)];
            size = 1ULL << PT_SHIFT;
        }
         
        if (pte & PTE_P && pte & PTE_SHARE) 
        {
            int res = fun((void *)start, (void *)(start + PAGE_SIZE), arg);
            if (res < 0) return res;
        }

        start += size;
    }

    return 0;
}

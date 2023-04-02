#include <inc/types.h>
#include <inc/assert.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/stdio.h>
#include <inc/x86.h>
#include <inc/uefi.h>
#include <kern/timer.h>
#include <kern/kclock.h>
#include <kern/picirq.h>
#include <kern/trap.h>
#include <kern/pmap.h>

#define kilo      (1000ULL)
#define Mega      (kilo * kilo)
#define Giga      (kilo * Mega)
#define Tera      (kilo * Giga)
#define Peta      (kilo * Tera)
#define ULONG_MAX ~0UL

#if LAB <= 6
/* Early variant of memory mapping that does 1:1 aligned area mapping
 * in 2MB pages. You will need to reimplement this code with proper
 * virtual memory mapping in the future. */
void *
mmio_map_region(physaddr_t pa, size_t size) {
    void map_addr_early_boot(uintptr_t addr, uintptr_t addr_phys, size_t sz);
    const physaddr_t base_2mb = 0x200000;
    uintptr_t org = pa;
    size += pa & (base_2mb - 1);
    size += (base_2mb - 1);
    pa &= ~(base_2mb - 1);
    size &= ~(base_2mb - 1);
    map_addr_early_boot(pa, pa, size);
    return (void *)org;
}
void *
mmio_remap_last_region(physaddr_t pa, void *addr, size_t oldsz, size_t newsz) {
    return mmio_map_region(pa, newsz);
}
#endif

struct Timer timertab[MAX_TIMERS];
struct Timer *timer_for_schedule;

struct Timer timer_hpet0 = {
        .timer_name = "hpet0",
        .timer_init = hpet_init,
        .get_cpu_freq = hpet_cpu_frequency,
        .enable_interrupts = hpet_enable_interrupts_tim0,
        .handle_interrupts = hpet_handle_interrupts_tim0,
};

struct Timer timer_hpet1 = {
        .timer_name = "hpet1",
        .timer_init = hpet_init,
        .get_cpu_freq = hpet_cpu_frequency,
        .enable_interrupts = hpet_enable_interrupts_tim1,
        .handle_interrupts = hpet_handle_interrupts_tim1,
};

struct Timer timer_acpipm = {
        .timer_name = "pm",
        .timer_init = acpi_enable,
        .get_cpu_freq = pmtimer_cpu_frequency,
};

void
acpi_enable(void) {
    FADT *fadt = get_fadt();
    outb(fadt->SMI_CommandPort, fadt->AcpiEnable);
    while ((inw(fadt->PM1aControlBlock) & 1) == 0) /* nothing */
        ;
}

static bool check_sum_zero(const void* addr, size_t size)
{
    assert(addr);
    unsigned char sum = 0;

    for (size_t iter = 0; iter < size; iter++)
    {
        unsigned char val = *((unsigned char*) addr + iter);
        sum = (unsigned char) (sum + val);
    }        

    return (sum == 0);
}

static void *
acpi_find_table(const char *sign) {
    /*
     * This function performs lookup of ACPI table by its signature
     * and returns valid pointer to the table mapped somewhere.
     *
     * It is a good idea to checksum tables before using them.
     *
     * HINT: Use mmio_map_region/mmio_remap_last_region
     * before accessing table addresses
     * (Why mmio_remap_last_region is requrired?)
     * HINT: RSDP address is stored in uefi_lp->ACPIRoot
     * HINT: You may want to distunguish RSDT/XSDT
     */

    // LAB 5: Your code here

    assert(sign);

    RSDP* rsdp = get_rsdp();
    if (strncmp(RSDP_sign, sign, sizeof(RSDP_sign)) == 0)
        return rsdp;        

    // check revision

    XSDT* xsdt = get_xsdt(rsdp);
    if (strncmp(XSDT_sign, sign, sizeof(XSDT_sign)) == 0)
        return xsdt;

    uint64_t xsdt_num_entries = (xsdt->h.Length - sizeof(ACPISDTHeader)) / sizeof(uint64_t);

    for (uint64_t iter = 0; iter < xsdt_num_entries; iter++)
    {
        ACPISDTHeader* cur_dh    = (ACPISDTHeader*) xsdt->PointerToOtherSDT[iter];
        ACPISDTHeader* dh_mapped = (ACPISDTHeader*) mmio_map_region((physaddr_t) cur_dh, sizeof(ACPISDTHeader));

        if (strncmp(sign, dh_mapped->Signature, sizeof(dh_mapped->Signature)) != 0)
            continue;

        size_t table_size = (size_t) dh_mapped->Length;
        dh_mapped = (ACPISDTHeader*) mmio_remap_last_region((physaddr_t) cur_dh, dh_mapped, sizeof(ACPISDTHeader), table_size);

        if (!check_sum_zero(dh_mapped, table_size))
            panic("%s incorrect checksum\n", dh_mapped->Signature);

        return dh_mapped;      
    }

    return NULL;
}

/* Obtain and map FADT ACPI table address. */
FADT *
get_fadt(void) {
    // LAB 5: Your code here
    // (use acpi_find_table)
    // HINT: ACPI table signatures are
    //       not always as their names

    static FADT *kfadt = NULL;

    if (kfadt)
        return kfadt;

    kfadt = (FADT*) acpi_find_table(FADT_sign);
    return kfadt;
}

/* Obtain and map RSDP ACPI table address. */
HPET *
get_hpet(void) {
    // LAB 5: Your code here
    // (use acpi_find_table)

    static HPET *khpet = NULL;

    if (khpet)
        return khpet;

    khpet = (HPET*) acpi_find_table(HPET_sign);
    return khpet;
}

RSDP *
get_rsdp(void) {

    static RSDP* rsdp_saved = NULL;

    if (rsdp_saved)
        return rsdp_saved;

    // RSDP*  rsdp_ptr = (RSDP*) ((LOADER_PARAMS*) (KADDR((physaddr_t)uefi_lp)))->ACPIRoot;
    RSDP* rsdp_ptr = (RSDP*) uefi_lp->ACPIRoot;
    assert(rsdp_ptr); 

    size_t rsdp_size = sizeof(RSDP);
    size_t rsdp_first_checksum_size = 20UL;

    RSDP* rsdp_mapped = (RSDP*) mmio_map_region((physaddr_t) rsdp_ptr, rsdp_size); 

    if (strncmp(rsdp_mapped->Signature, RSDP_sign, sizeof(RSDP_sign)))
        panic("RSDP incorrect signature\n");

    if (!check_sum_zero(rsdp_mapped, rsdp_first_checksum_size))
        panic("RSDP first checksum failed\n");

    if (!check_sum_zero(rsdp_mapped, rsdp_size))
        panic("RSDP second checksum failed\n");

    rsdp_saved = rsdp_mapped;
    return rsdp_mapped;
}

XSDT* 
get_xsdt(RSDP* rsdp) {

    assert(rsdp);

    static XSDT* xsdt_saved = NULL;

    if (xsdt_saved)
        return xsdt_saved;

    XSDT* xsdt_addr   = (XSDT*) rsdp->XsdtAddress;
    XSDT* xsdt_mapped = (XSDT*) mmio_map_region((physaddr_t) xsdt_addr, sizeof(ACPISDTHeader));

    if (strncmp(xsdt_mapped->h.Signature, XSDT_sign, sizeof(XSDT_sign)))
        panic("XSDT incorrect signature\n");    

    uint32_t xsdt_len = xsdt_mapped->h.Length;
    xsdt_mapped = (XSDT*) mmio_remap_last_region((physaddr_t) xsdt_addr, xsdt_mapped, sizeof(ACPISDTHeader), xsdt_len);

    if (!check_sum_zero(xsdt_mapped, xsdt_len))
        panic("XSDT incorrect checksum\n");

    xsdt_saved = xsdt_mapped;
    return xsdt_mapped;
}

/* Getting physical HPET timer address from its table. */
HPETRegister *
hpet_register(void) {
    HPET *hpet_timer = get_hpet();
    if (!hpet_timer->address.address) panic("hpet is unavailable\n");

    uintptr_t paddr = hpet_timer->address.address;
    return mmio_map_region(paddr, sizeof(HPETRegister));
}

/* Debug HPET timer state. */
void
hpet_print_struct(void) {
    HPET *hpet = get_hpet();
    assert(hpet != NULL);
    cprintf("signature = %s\n", (hpet->h).Signature);
    cprintf("length = %08x\n", (hpet->h).Length);
    cprintf("revision = %08x\n", (hpet->h).Revision);
    cprintf("checksum = %08x\n", (hpet->h).Checksum);

    cprintf("oem_revision = %08x\n", (hpet->h).OEMRevision);
    cprintf("creator_id = %08x\n", (hpet->h).CreatorID);
    cprintf("creator_revision = %08x\n", (hpet->h).CreatorRevision);

    cprintf("hardware_rev_id = %08x\n", hpet->hardware_rev_id);
    cprintf("comparator_count = %08x\n", hpet->comparator_count);
    cprintf("counter_size = %08x\n", hpet->counter_size);
    cprintf("reserved = %08x\n", hpet->reserved);
    cprintf("legacy_replacement = %08x\n", hpet->legacy_replacement);
    cprintf("pci_vendor_id = %08x\n", hpet->pci_vendor_id);
    cprintf("hpet_number = %08x\n", hpet->hpet_number);
    cprintf("minimum_tick = %08x\n", hpet->minimum_tick);

    cprintf("address_structure:\n");
    cprintf("address_space_id = %08x\n", (hpet->address).address_space_id);
    cprintf("register_bit_width = %08x\n", (hpet->address).register_bit_width);
    cprintf("register_bit_offset = %08x\n", (hpet->address).register_bit_offset);
    cprintf("address = %08lx\n", (unsigned long)(hpet->address).address);
}

static volatile HPETRegister *hpetReg;
/* HPET timer period (in femtoseconds) */
static uint64_t hpetFemto = 0;
/* HPET timer frequency */
static uint64_t hpetFreq = 0;

/* HPET timer initialisation */
void
hpet_init() {
    if (hpetReg == NULL) {
        nmi_disable();
        hpetReg = hpet_register();
        uint64_t cap = hpetReg->GCAP_ID;
        hpetFemto = (uintptr_t)(cap >> 32);
        if (!(cap & HPET_LEG_RT_CAP)) panic("HPET has no LegacyReplacement mode");

        cprintf("hpetFemto = %lu\n", hpetFemto); 
        hpetFreq = (1 * Peta) / hpetFemto;
         cprintf("HPET: Frequency = %ld.%03ldMHz\n", (uintptr_t)(hpetFreq / Mega), (uintptr_t)(hpetFreq % Mega)); 
        /* Enable ENABLE_CNF bit to enable timer */
        hpetReg->GEN_CONF |= HPET_ENABLE_CNF;
        nmi_enable();
    }
}

/* HPET register contents debugging. */
void
hpet_print_reg(void) {
    cprintf("GCAP_ID = %016lx\n", (unsigned long)hpetReg->GCAP_ID);
    cprintf("GEN_CONF = %016lx\n", (unsigned long)hpetReg->GEN_CONF);
    cprintf("GINTR_STA = %016lx\n", (unsigned long)hpetReg->GINTR_STA);
    cprintf("MAIN_CNT = %016lx\n", (unsigned long)hpetReg->MAIN_CNT);
    cprintf("TIM0_CONF = %016lx\n", (unsigned long)hpetReg->TIM0_CONF);
    cprintf("TIM0_COMP = %016lx\n", (unsigned long)hpetReg->TIM0_COMP);
    cprintf("TIM0_FSB = %016lx\n", (unsigned long)hpetReg->TIM0_FSB);
    cprintf("TIM1_CONF = %016lx\n", (unsigned long)hpetReg->TIM1_CONF);
    cprintf("TIM1_COMP = %016lx\n", (unsigned long)hpetReg->TIM1_COMP);
    cprintf("TIM1_FSB = %016lx\n", (unsigned long)hpetReg->TIM1_FSB);
    cprintf("TIM2_CONF = %016lx\n", (unsigned long)hpetReg->TIM2_CONF);
    cprintf("TIM2_COMP = %016lx\n", (unsigned long)hpetReg->TIM2_COMP);
    cprintf("TIM2_FSB = %016lx\n", (unsigned long)hpetReg->TIM2_FSB);
}

/* HPET main timer counter value. */
uint64_t
hpet_get_main_cnt(void) {
    return hpetReg->MAIN_CNT;
}

/* - Configure HPET timer 0 to trigger every 0.5 seconds on IRQ_TIMER line
 * - Configure HPET timer 1 to trigger every 1.5 seconds on IRQ_CLOCK line
 *
 * HINT To be able to use HPET as PIT replacement consult
 *      LegacyReplacement functionality in HPET spec.
 * HINT Don't forget to unmask interrupt in PIC */
void
hpet_enable_interrupts_tim0(void) {
    // LAB 5: Your code here

    nmi_disable();
    hpetReg->GEN_CONF &= (~HPET_ENABLE_CNF);                        // disable HPET
    hpetReg->GEN_CONF |= HPET_LEG_RT_CNF;                           // enable LegacyReplacement

    if (!(hpetReg->TIM0_CONF & HPET_TN_PER_INT_CAP))
        panic("Timer 0 doesn't support periodic interrupts\n");     // panic if periodic interrupts are not supported
    hpetReg->TIM0_CONF |= HPET_TN_TYPE_CNF;                         // enable periodic interrupts

    hpetReg->TIM0_CONF |= HPET_TN_INT_ENB_CNF;                      // set to enable timer to generate interrupts 

    // After that, we need to write down value to counter as it's described in specification

    hpetReg->MAIN_CNT = 0;                                          // set main ct to zero
    hpetReg->TIM0_CONF |= HPET_TN_VAL_SET_CNF;                      // enable to set comparators value

    hpetReg->TIM0_COMP = Peta / (2 * hpetFemto);                    // set appropiate comparator value

    hpetReg->GEN_CONF |= HPET_ENABLE_CNF;                           // enable HPET
    
    pic_irq_unmask(IRQ_TIMER);                                      // unmask 
    nmi_enable();                                                   
}

void
hpet_enable_interrupts_tim1(void) {
    // LAB 5: Your code here

    nmi_disable();
    hpetReg->GEN_CONF &= (~HPET_ENABLE_CNF);                        // disable HPET
    hpetReg->GEN_CONF |= HPET_LEG_RT_CNF;                           // enable LegacyReplacement

    if (!(hpetReg->TIM1_CONF & HPET_TN_PER_INT_CAP))
        panic("Timer 1 doesn't support periodic interrupts\n");    // panic if periodic interrupts are not supported
    hpetReg->TIM1_CONF |= HPET_TN_TYPE_CNF;                         // enable periodic interrupts

    hpetReg->TIM1_CONF |= HPET_TN_INT_ENB_CNF;                      // set to enable timer to generate interrupts 

    // After that, we need to write down value to counter as it's described in specification

    hpetReg->MAIN_CNT = 0;                                          // set main ct to zero
    hpetReg->TIM1_CONF |= HPET_TN_VAL_SET_CNF;                      // enable to set comparators value

    hpetReg->TIM1_COMP = (3 *Peta) / (2 * hpetFemto);               // set appropiate comparator value

    hpetReg->GEN_CONF |= HPET_ENABLE_CNF;                           // enable HPET
    
    pic_irq_unmask(IRQ_CLOCK);                                      // unmask 
    nmi_enable();   
}

void
hpet_handle_interrupts_tim0(void) {
    pic_send_eoi(IRQ_TIMER);
}

void
hpet_handle_interrupts_tim1(void) {
    pic_send_eoi(IRQ_CLOCK);
}

/* Calculate CPU frequency in Hz with the help with HPET timer.
 * HINT Use hpet_get_main_cnt function and do not forget about
 * about pause instruction. */
uint64_t
hpet_cpu_frequency(void) {
    static uint64_t cpu_freq;

    // LAB 5: Your code here

    uint64_t tsc1 = 0, tsc2 = 0;
    const uint64_t measurement = 100000;

    uint64_t hpet_start_main_cnt = hpetReg->MAIN_CNT;
    uint64_t hpet_delta = 0;
    
    tsc1 = read_tsc();

    do 
    {
        asm volatile("pause");
        hpet_delta = hpetReg->MAIN_CNT - hpet_start_main_cnt;

    } while (hpet_delta < measurement);
    
    tsc2 = read_tsc();

    uint64_t tsc_delta = tsc2 - tsc1;
    cpu_freq = hpetFreq * tsc_delta / hpet_delta;

    return cpu_freq;
}

uint32_t
pmtimer_get_timeval(void) {
    FADT *fadt = get_fadt();
    return inl(fadt->PMTimerBlock);
}

/* Calculate CPU frequency in Hz with the help with ACPI PowerManagement timer.
 * HINT Use pmtimer_get_timeval function and do not forget that ACPI PM timer
 *      can be 24-bit or 32-bit. */
uint64_t
pmtimer_cpu_frequency(void) {
    static uint64_t cpu_freq;

    // LAB 5: Your code here

    // Firstly we need to get size of ACPI PM timer in bits
    // 0 - 24 bits, 1 - 32 bits
    FADT* fadt = get_fadt();
    bool tmr_val_ext_flag_set = (fadt->Flags & ACPI_FADT_FLAG_TMR_VAL_EXT);

    uint64_t overflow_value = (tmr_val_ext_flag_set)? 0xFFFFFFFF : 0xFFFFFF;

    uint64_t tsc1 = 0, tsc2    = 0;
    const uint64_t measurement = 10000000;

    uint64_t pmtimer_start = (uint64_t) pmtimer_get_timeval();
    uint64_t pmtimer_delta = 0;

    uint64_t overflow_delta = 0;
    uint64_t prev_pmtimer   = 0;

    tsc1 = read_tsc();

    do 
    {
        asm volatile("pause");

        uint64_t cur_pmtimer = (uint64_t) pmtimer_get_timeval();

        if (cur_pmtimer < prev_pmtimer)
        {
            cprintf("pm timer overflow \n\n");
            overflow_delta += overflow_value;
        }

        prev_pmtimer  = cur_pmtimer;
        pmtimer_delta = cur_pmtimer + overflow_delta - pmtimer_start;

    } while (pmtimer_delta < measurement);
    
    tsc2 = read_tsc(); 

    uint64_t tsc_delta = tsc2 - tsc1;
    cpu_freq = PM_FREQ * tsc_delta / pmtimer_delta;

    // cprintf("tsc1 %lu tsc2  %lu pmtimer_delta %lu overflow_delta %lu overflow_value %lu\n", tsc1, tsc2, pmtimer_delta, overflow_delta, overflow_value);

    return cpu_freq;
}

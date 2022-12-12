#ifndef JOS_KERN_PCI_H
#define JOS_KERN_PCI_H
#ifndef JOS_KERNEL
#error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/assert.h>
#include <inc/x86.h>
#include <inc/types.h>

#define MAX_BUS 256
#define MAX_DEV 32
#define MAX_FUN 8

#define PCI_CONFIGURATION_ADDR_PORT  0xCF8
#define PCI_CONFIGURATION_DATA_PORT  0xCFC

/* PCI config addr port layout: 

    register_offset: 8;  7  - 0  
    function_number: 3;  10 - 8 
    device_number  : 5;  15 - 11
    bus_number     : 8;  23 - 16
    reserved       : 7;  30 - 24
    enable_bit     : 1;  31 
*/

#define PCI_REGISTER_OFFS_MASK 0xFC              // alignment

#define PCI_CONF_ADDR_PORT_ENABLE_BIT 0x80000000 // enable bit
#define PCI_CONF_ADDR_PORT_BUS_OFFS   16         // offset
#define PCI_CONF_ADDR_PORT_DEV_OFFS   11         // offset
#define PCI_CONF_ADDR_PORT_FUN_OFFS   8          // offset

typedef struct
{
    uint8_t bus_number;
    uint8_t device_number;
    uint8_t function_number;

} pci_dev_t;

/* Common header fields layout */

/*      field name                            offs   size  reg */
#define PCI_CONF_SPACE_VENDOR_ID              0x00 // 2    0
#define PCI_CONF_SPACE_DEVICE_ID              0x02 // 2    0
#define PCI_CONF_SPACE_COMMAND                0x04 // 2    1
#define PCI_CONF_SPACE_STATUS                 0x06 // 2    1
#define PCI_CONF_SPACE_REVISION_ID            0x08 // 1    2
#define PCI_CONF_SPACE_PROG_IF                0x09 // 1    2
#define PCI_CONF_SPACE_SUBCLASS               0x0A // 1    2
#define PCI_CONF_SPACE_CLASS                  0x0B // 1    2
#define PCI_CONF_SPACE_CACHE_LINE_SIZE        0x0C // 1    3
#define PCI_CONF_SPACE_LATENCY_TIMER          0x0D // 1    3
#define PCI_CONF_SPACE_HEADER_TYPE            0x0E // 1    3
#define PCI_CONF_SPACE_BIST                   0x0F // 1    3

/* Header type 0 specific fields */

#define PCI_CONF_SPACE_TYPE_0_BAR0            0x10 // 4    4
#define PCI_CONF_SPACE_TYPE_0_BAR1            0x14 // 4    5
#define PCI_CONF_SPACE_TYPE_0_BAR2            0x18 // 4    6
#define PCI_CONF_SPACE_TYPE_0_BAR3            0x1C // 4    7
#define PCI_CONF_SPACE_TYPE_0_BAR4            0x20 // 4    8
#define PCI_CONF_SPACE_TYPE_0_BAR5            0x24 // 4    9
#define PCI_CONF_SPACE_TYPE_0_CARDBUS_CIS     0x28 // 4    A
#define PCI_CONF_SPACE_TYPE_0_SUB_VENDOR_ID   0x2C // 2    B
#define PCI_CONF_SPACE_TYPE_0_SUB_DEVICE_ID   0x2E // 2    B
#define PCI_CONF_SPACE_TYPE_0_EXPANSION_ROM   0x30 // 4    C
#define PCI_CONF_SPACE_TYPE_0_CAPABILITIES    0x34 // 1    D
#define PCI_CONF_SPACE_TYPE_0_INTERRUPT_LINE  0x3C // 1    F
#define PCI_CONF_SPACE_TYPE_0_INTERRUPT_PIN   0x3D // 1    F
#define PCI_CONF_SPACE_TYPE_0_MIN_GNT         0x3E // 1    F
#define PCI_CONF_SPACE_TYPE_0_MAX_LAT         0x3F // 1    F

/* Header type 1 (PCI-to-PCI bridge) specific fields */

#define PCI_CONF_SPACE_TYPE_1_BAR0            0x10 // 4    4
#define PCI_CONF_SPACE_TYPE_1_BAR1            0x14 // 4    5
#define PCI_CONF_SPACE_TYPE_1_PRIM_BUS_NUM    0x18 // 1    6
#define PCI_CONF_SPACE_TYPE_1_SECN_BUS_NUM    0x19 // 1    6
#define PCI_CONF_SPACE_TYPE_1_SUBR_BUS_NUM    0x1A // 1    6
#define PCI_CONF_SPACE_TYPE_1_SECN_LAT_TIM    0x1B // 1    6
#define PCI_CONF_SPACE_TYPE_1_IO_BASE         0x1C // 1    7
#define PCI_CONF_SPACE_TYPE_1_IO_LIM          0x1D // 1    7
#define PCI_CONF_SPACE_TYPE_1_SECN_STATUS     0x1E // 2    7
#define PCI_CONF_SPACE_TYPE_1_MEM_BASE        0x20 // 2    8
#define PCI_CONF_SPACE_TYPE_1_MEM_LIM         0x22 // 2    8
#define PCI_CONF_SPACE_TYPE_1_PREF_MEM_BASE   0x24 // 2    9
#define PCI_CONF_SPACE_TYPE_1_PREF_MEM_LIM    0x26 // 2    9
#define PCI_CONF_SPACE_TYPE_1_PREF_BASE_U32   0x28 // 4    A
#define PCI_CONF_SPACE_TYPE_1_PREF_BASE_U32   0x2C // 4    B
#define PCI_CONF_SPACE_TYPE_1_IO_BASE_U16     0x30 // 2    C
#define PCI_CONF_SPACE_TYPE_1_IO_LIM_U16      0x32 // 2    C
#define PCI_CONF_SPACE_TYPE_1_CAPABILITIES    0x34 // 1    D
#define PCI_CONF_SPACE_TYPE_1_EXPANSION_ROM   0x38 // 4    E
#define PCI_CONF_SPACE_TYPE_1_INTERRUPT_LINE  0x3C // 1    F
#define PCI_CONF_SPACE_TYPE_1_INTERRUPT_PIN   0x3D // 1    F
#define PCI_CONF_SPACE_TYPE_1_BRIDGE_CTRL     0x3E // 2    F

/* Header Type Register layout */

#define HEADER_TYPE_REG_MF 0x80 // Multiple functions flag
#define HEADER_TYPE_REG_HT 0x7F // Header type

/* BIST Register layout */

#define BIST_REG_CAPABLE 0x80 // BIST support
#define BIST_REG_START   0x40 // BIST invoked, reset on complete (timeout == 2 sec)
#define BIST_REG_RES     0x30 // reserved
#define BIST_REG_COMPLT  0xF  // returns 0 if the test completed successfully

/* Command register layout */

#define CMND_REG_RES1        0xF800 // reserved
#define CMND_REG_INT_OFF     0x400  // interrupt disable if set to 1
#define CMND_REG_FBB_ON      0x200  // fast back-to-back transactions are allowed
#define CMND_REG_SERR_ON     0x100  // SERR driver is enabled
#define CMND_REG_RES2        0x80   // reserved
#define CMND_REG_PAR_ERR     0x40   // parity error - normal action
#define CMND_REG_VGA_PS      0x20   // VGA palette snoop 
#define CMND_REG_MWI_ON      0x10   // device can generate memory write & invalidate cmd
#define CMND_REG_SPEC_CYCLES 0x8    // device can monitor special cycles operations
#define CMND_REG_BUS_MASTER  0x4    // device can act as bus master
#define CMND_REG_MEM_SPACE   0x2    // device can respond to memory space accesses
#define CMND_REG_IO_SPACE    0x1    // device can respond to io space accesses

/* Status register layout*/

#define STATUS_REG_DET_PAR_ERR         0x8000 // 1 if device detects a parity error, even if parity error handling is disabled.
#define STATUS_REG_SYG_SYS_ERR         0x4000 // 1 if the device asserts SERR#.
#define STATUS_REG_RCV_MASTER_ABRT     0x2000 // 1 set by a master device, whenever its transaction (except for Special Cycle transactions) is terminated with Master-Abort.
#define STATUS_REG_RCV_TARGET_ABRT     0x1000 // 1 set by a master device, whenever its transaction is terminated with Target-Abort.
#define STATUS_REG_SYG_TARGET_ABRT     0x800  // target device terminates a transaction with Target-Abort
#define STATUS_REG_DEVSEL_TIMING       0x600  // the slowest time that a device will assert DEVSEL# for any bus command except Configuration Space read and writes. 0x0 - fast timing, 0x1 - medium, 0x2 - slow
#define STATUS_REG_MASTER_DATA_PAR_ERR 0x100  // This bit is only set when the following conditions are met. The bus agent asserted PERR# on a read or observed an assertion of PERR# on a write, the agent setting the bit acted as the bus master for the operation in which the error occurred, and bit 6 of the Command register (Parity Error Response bit) is set to 1.
#define STATUS_REG_FAST_BTB_CAPABLE    0x80   // device can accept fast back-to-back transactions that are not from the same agent; otherwise, transactions can only be accepted from the same agent
#define STATUS_REG_RES1                0x40   // reserved
#define STATUS_REG_66_MHZ_CAPABLE      0x20   // device is capable of running at 66 MHz; otherwise, the device runs at 33 MHz.
#define STATUS_REG_CAPABILITIES_LIST   0x10   // device implements the pointer for a New Capabilities Linked list at offset 0x34; otherwise, the linked list is not available.
#define STATUS_REG_INTERRUPT_STATUS    0x8    // 
#define STATUS_REG_RES2                0x7    // reserved

/* Memory Space BAR Layout */

#define MS_BAR_BASE_ADDR 0xFFFFFFF0 // 
#define MS_BAR_PREFETCH  0x8        // does not have read side effects 
#define MS_BAR_TYPE      0x6        // 0 - 32 bit , 2 - 64 bit, 1 - reserved 
#define MS_BAR_ALWAYS0   0x1        // to distinguish

/* I/O Space BAR Layout */

#define IOS_BAR_BASE_ADDR 0xFFFFFFFC // base address
#define IOS_BAR_RES       0x2        // reserved
#define IOS_BAR_ALWAYS1   0x1        // to distinguish

/* Class codes*/

#define PCI_CLASS_UNCLASSIFIED            0x0
#define PCI_CLASS_MASS_STORAGE_CONTROLLER 0x1
#define PCI_CLASS_NETWORK_CONTROLLER      0x2
#define PCI_CLASS_DISPLAY_CONTROLLER      0x3
#define PCI_CLASS_MULTIMEDIA_CONTROLLER   0x4
#define PCI_CLASS_MEMORY_CONTROLLER       0x5
#define PCI_CLASS_BRIDGE                  0x6
#define PCI_CLASS_SIMPLE_COMMUNICATION    0x7
#define PCI_CLASS_BASE_SYS_PERIPHERAL     0x8
#define PCI_CLASS_IMPUT_DEVICE_CONTOLLER  0x9
#define PCI_CLASS_DOCKING_STATION         0xA
#define PCI_CLASS_PROCESSOR               0xB
#define PCI_CLASS_SERIAL_BUS_CONTROLLER   0xC
#define PCI_CLASS_WIRELESS_CONTROLLER     0xD
#define PCI_CLASS_INTELLIGENT_CONTROLLER  0xE
#define PCI_CLASS_SATELLITE_COMMUNICATION 0xF
#define PCI_CLASS_ENCRYPTION_CONTROLLER   0x10
#define PCI_CLASS_SIGNAL_PROCESSING       0x11
#define PCI_CLASS_PROCESSING_ACCELERATOR  0x12
#define PCI_CLASS_NON_ESSENTIAL_INSRT     0x13
#define PCI_CLASS_CO_PROCESSOR            0x40 
#define PCI_CLASS_UNASSIGNED_CLASS        0xFF

/* Network controller subclasses */

#define PCI_SUBCLASS_ETHERNET   0x0
#define PCI_SUBCLASS_TOKEN_RING 0x1
#define PCI_SUBCLASS_FDDI       0x2
#define PCI_SUBCLASS_ATM        0x3
#define PCI_SUBCLASS_ISDN       0x4
#define PCI_SUBCLASS_WORLDFLIP  0x5
#define PCI_SUBCLASS_PICMG_2_14 0x6
#define PCI_SUBCLASS_INFINIBAND 0x7
#define PCI_SUBCLASS_FABRIC     0x8
#define PCI_SUBCLASS_OTHER      0x80

/* Enumerating PCI buses */

bool check_pci_device(uint8_t bus, uint8_t device, uint8_t function);
int  enumerate_pci_devices(void);

/* Read-write functions for PIC configuration space */

uint8_t  pci_config_read8 (const pci_dev_t* pci_dev, uint8_t offset);
uint16_t pci_config_read16(const pci_dev_t* pci_dev, uint8_t offset); 
uint32_t pci_config_read32(const pci_dev_t* pci_dev, uint8_t offset); 

void pci_config_write8 (const pci_dev_t* pci_dev, uint8_t offset, uint8_t  val);
void pci_config_write16(const pci_dev_t* pci_dev, uint8_t offset, uint16_t val); 
void pci_config_write32(const pci_dev_t* pci_dev, uint8_t offset, uint32_t val); 

void init_pci(void);

#endif /* !JOS_KERN_PCI_H */
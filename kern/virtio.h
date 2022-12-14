#ifndef JOS_KERN_VIRTIO_H
#define JOS_KERN_VIRTIO_H
#ifndef JOS_KERNEL
#error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/pci.h>

const static uint16_t Virtio_pci_vendor_id = 0x1AF4;

#define PCI_DEVICE_ID_CALC_OFFSET 0x1040

/* For device configuration access, the driver MUST use 8-bit wide accesses 
 * for 8-bit wide fields, 16-bit wide and aligned accesses for 16-bit wide 
 * fields and 32-bit wide and aligned accesses for 32-bit and 64-bit wide 
 * fields. For 64-bit fields, the driver MAY access each of the high and 
 * low 32-bit parts of the field independently. 
*/

// write offsets in every structure TODO

//      Field name                     Offs  Size
#define VIRTIO_CMN_CFG_DEVICE_F_SELECT 0x0   // 4
#define VIRTIO_CMN_CFG_DEVICE_F        0x4   // 4
#define VIRTIO_CMN_CFG_DRIVER_F_SELECT 0x8   // 4
#define VIRTIO_CMN_CFG_DRIVER_F        0xC   // 4
#define VIRTIO_CMN_CFG_MSIX_CONFIG     0x10  // 2
#define VIRTIO_CMN_CFG_NUM_QUEUES      0x12  // 2
#define VIRTIO_CMN_CFG_DEV_STATUS      0x14  // 1
#define VIRTIO_CMN_CFG_CONF_GEN        0x15  // 1

#define VIRTIO_CMN_CFG_Q_SELECT        0x16  // 2
#define VIRTIO_CMN_CFG_Q_SIZE          0x18  // 2
#define VIRTIO_CMN_CFG_Q_MSIX_VECTOR   0x1A  // 2
#define VIRTIO_CMN_CFG_Q_ENABLE        0x1C  // 2
#define VIRTIO_CMN_CFG_Q_NOTIFY_OFF    0x1E  // 2
#define VIRTIO_CMN_CFG_Q_DESC          0x20  // 8
#define VIRTIO_CMN_CFG_Q_DRIVER        0x28  // 8
#define VIRTIO_CMN_CFG_Q_DEVICE        0x30  // 8

#define VIRTIO_ISR_QUEUE_INT           0x1
#define VIRTIO_ISR_DEVICE_CONF_INT     0x2

/* The device MUST allow reading of any device-specific configuration field 
 * before FEATURES_OK is set by the driver 
*/

/* Device status flags                       Offs */
#define VIRTIO_PCI_STATUS_ACKNOWLEDGE        0x1   
#define VIRTIO_PCI_STATUS_DRIVER             0x2   
#define VIRTIO_PCI_STATUS_DRIVER_OK          0x4
#define VIRTIO_PCI_STATUS_FEATURES_OK        0x20
#define VIRTIO_PCI_STATUS_DEVICE_NEEDS_RESET 0x40  
#define VIRTIO_PCI_STATUS_FAILED             0x80  // driver has given up on the device 

/* This marks a buffer as continuing via the next field. */ 
#define VIRTQ_DESC_F_NEXT     1 

/* This marks a buffer as device write-only (otherwise device read-only). */ 
#define VIRTQ_DESC_F_WRITE    2 

/* This means the buffer contains a list of buffer descriptors. */ 
#define VIRTQ_DESC_F_INDIRECT 4

typedef struct Vring_desc
{
	uint64_t addr;
	uint32_t len;
	uint16_t flags; // above
	uint16_t next;

} vring_desc_t;

typedef struct Vring_avail
{
    uint16_t flags;
	uint16_t idx;
	uint16_t ring[];
    /* uint16_t used_event; Only if VIRTIO_F_EVENT_IDX */ 

} vring_avail_t;

typedef struct Vring_used_elem
{
	uint32_t index;  // Index of start of used descriptor chain.
	uint32_t length; // Total length of the descriptor chain which was used (written to)

} vring_used_elem_t;


#define VIRTQ_USED_F_NO_NOTIFY  1 

typedef struct Vring_used
{
    uint16_t flags;
	uint16_t idx;
	vring_used_elem_t ring[];

} vring_used_t;

typedef struct Vring
{
    unsigned num;
    
    vring_desc_t*  desc;
    vring_avail_t* avail;
    vring_used_t*  used;

} vring_t;

typedef struct Virtqueue
{
    vring_t vring;

    uint16_t free_index;     // index of first free descriptor
    uint16_t num_free;       // count of free descriptors
    uint16_t last_seen_used; // last seen used

} virtqueue_t;



typedef struct Virtio_pci_cap 
{ 
    uint8_t cap_vndr;    /* Generic PCI field: PCI_CAP_ID_VNDR */ 
    uint8_t cap_next;    /* Generic PCI field: next ptr. */ 
    uint8_t cap_len;     /* Generic PCI field: capability length */ 
    uint8_t cfg_type;    /* Identifies the structure. */ 
    uint8_t bar;         /* Where to find it. */ 
    uint8_t padding[3];  /* Pad to full dword. */ 
    uint32_t offset;    /* Offset within bar. */ 
    uint32_t length;    /* Length of the structure, in bytes. */ 

} virtio_pci_cap_t;

//      Filed name            Offs Size
#define VIRTIO_PCI_CAP_VNDR   0x0  // 1
#define VIRTIO_PCI_CAP_NEXT   0x1  // 1
#define VIRTIO_PCI_CAP_LEN    0x2  // 1
#define VIRTIO_PCI_CAP_TYPE   0x3  // 1
#define VIRTIO_PCI_CAP_BAR    0x4  // 1
#define VIRTIO_PCI_CAP_PDNG   0x5  // 3
#define VIRTIO_PCI_CAP_OFFS   0x8  // 4
#define VIRTIO_PCI_CAP_LENGTH 0x12 // 4

/* Common configuration */ 
#define VIRTIO_PCI_CAP_COMMON_CFG        1 
/* Notifications */ 
#define VIRTIO_PCI_CAP_NOTIFY_CFG        2 
/* ISR Status */ 
#define VIRTIO_PCI_CAP_ISR_CFG           3 
/* Device specific configuration */ 
#define VIRTIO_PCI_CAP_DEVICE_CFG        4 
/* PCI configuration access */ 
#define VIRTIO_PCI_CAP_PCI_CFG           5

#define VIRTIO_PCI_CAP_MAX 5
#define VIRTIO_PCI_CAP_MIN 1

typedef struct Virtio_pci_common_cfg 
{ 
    /* About the whole device. */ 
    uint32_t device_feature_select;     /* read-write */ 
    uint32_t device_feature;            /* read-only for driver */ 
    uint32_t driver_feature_select;     /* read-write */ 
    uint32_t driver_feature;            /* read-write */ 
    uint16_t msix_config;               /* read-write */ 
    uint16_t num_queues;                /* read-only for driver */ 
    uint8_t device_status;               /* read-write */ 
    uint8_t config_generation;           /* read-only for driver */ 

    /* About a specific virtqueue. */ 
    uint16_t queue_select;              /* read-write */ 
    uint16_t queue_size;                /* read-write */ 
    uint16_t queue_msix_vector;         /* read-write */ 
    uint16_t queue_enable;              /* read-write */ 
    uint16_t queue_notify_off;          /* read-only for driver */ 
    uint64_t queue_desc;                /* read-write */ 
    uint64_t queue_driver;              /* read-write */ 
    uint64_t queue_device;              /* read-write */ 

} virtio_pci_common_cfg_t;

typedef struct Virtio_pci_notify_cap 
{ 
    virtio_pci_cap_t cap; 
    uint32_t notify_off_multiplier; /* Multiplier for queue_notify_off. */ 

} virtio_pci_notify_cap_t;

typedef struct Virtio_pci_isr_cfg
{
    unsigned queue_int: 1;
    unsigned device_conf_int: 1;
    unsigned reserved: 30;

} virtio_pci_isr_cfg_t;

typedef struct Virtio_pci_cfg_cap 
{ 
    virtio_pci_cap_t cap; 
    uint8_t pci_cfg_data[4]; /* Data for BAR access. */ 

} virtio_pci_cfg_cap_t;



typedef struct Virtio_nic_dev
{
    pci_dev_general_t pci_dev_general;

    uint64_t rcv_queue_addr;
    uint64_t snd_queue_addr;
    uint64_t ctrl_queue_addr; // if supported

    uint32_t features; // features supported by both device & driver

    virtio_pci_cap_t capabilities[5];
    bool pci_conf_acc_io; // true - PCI configuration access capability is accessed through IO space
 
} virtio_nic_dev_t;



void read_virtio_pci_cap(pci_dev_t* pci_dev, virtio_pci_cap_t* virtio_pci_cap, uint8_t offs);

uint8_t  virtio_read8 (const virtio_nic_dev_t* virtio_nic_dev, uint8_t type, uint32_t offs);
uint16_t virtio_read16(const virtio_nic_dev_t* virtio_nic_dev, uint8_t type, uint32_t offs);
uint32_t virtio_read32(const virtio_nic_dev_t* virtio_nic_dev, uint8_t type, uint32_t offs);

void virtio_write8 (const virtio_nic_dev_t* virtio_nic_dev, uint8_t type, uint32_t offs, uint8_t  value);
void virtio_write16(const virtio_nic_dev_t* virtio_nic_dev, uint8_t type, uint32_t offs, uint16_t value);
void virtio_write32(const virtio_nic_dev_t* virtio_nic_dev, uint8_t type, uint32_t offs, uint32_t value);

void virtio_set_dev_status_flag  (const pci_dev_general_t* virtio_nic_dev, uint8_t flag);
bool virtio_check_dev_status_flag(const pci_dev_general_t* virtio_nic_dev, uint8_t flag);

bool virtio_check_dev_feature(const pci_dev_general_t* virtio_nic_dev, uint8_t feature);
void virtio_set_guest_feature(      pci_dev_general_t* virtio_nic_dev, uint8_t feature);

#endif /* !JOS_KERN_VIRTIO_H */
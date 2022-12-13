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

/* Virtio I/O registers            Offs  Size  Descr*/
#define VIRTIO_PCI_DEVICE_FEATURES 0x0   // 4  pre-configured by device 
#define VIRTIO_PCI_GUEST_FEATURES  0x4   // 4  used by guest VM to communicate the features supported by VM
#define VIRTIO_PCI_QUEUE_ADDR      0x8   // 4  
#define VIRTIO_PCI_QUEUE_SIZE      0xC   // 2  
#define VIRTIO_PCI_QUEUE_SELECT    0xE   // 2  
#define VIRTIO_PCI_QUEUE_NOTIFY    0x10  // 2  
#define VIRTIO_PCI_DEVICE_STATUS   0x12  // 1  current state of driver 
#define VIRTIO_PCI_ISR_STATUS      0x13  // 1  

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

} vring_avail_t;

typedef struct Vring_used_elem
{
	uint32_t index;  // Index of start of used descriptor chain.
	uint32_t length; // Total length of the descriptor chain which was used (written to)

} vring_used_elem_t;

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

uint32_t virtio_get_reg          (const pci_dev_general_t* virtio_nic_dev, uint32_t offs);

void virtio_set_dev_status_flag  (const pci_dev_general_t* virtio_nic_dev, uint8_t flag);
bool virtio_check_dev_status_flag(const pci_dev_general_t* virtio_nic_dev, uint8_t flag);

bool virtio_check_dev_feature    (const pci_dev_general_t* virtio_nic_dev, uint8_t feature);
void virtio_set_guest_feature    (      pci_dev_general_t* virtio_nic_dev, uint8_t feature);

#endif /* !JOS_KERN_VIRTIO_H */
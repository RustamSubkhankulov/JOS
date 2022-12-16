#ifndef JOS_KERN_VIRTIO_H
#define JOS_KERN_VIRTIO_H
#ifndef JOS_KERNEL
#error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/pci.h>
#include <kern/pmap.h>

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

/* ISR_STATUS Flags - sued to distinguish between device-specific 
   configuration change interrupts and normal virtqueue interrupts*/
#define ISR_STATUS_QUEUE_INT       0x1
#define ISR_STATUS_DEVICE_CONF_INT 0x2

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
    uint16_t num;
    
    vring_desc_t*  desc;
    vring_avail_t* avail;
    vring_used_t*  used;

} vring_t;

typedef struct Virtqueue
{
    vring_t vring;

    uint16_t free_index;     // index of first free descriptor
    uint16_t num_free;       // count of free descriptors 

    uint16_t last_used;      // last seen used
    uint16_t last_avail;

    uint8_t* copy_buf;
    uint32_t chunk_size;

} virtqueue_t;

#define MAX_VIRTQUEUE_SIZE 32768

typedef struct Virtio_dev
{
    pci_dev_general_t pci_dev_general;

    unsigned queues_n;
    virtqueue_t* queues;
    
    uint32_t features; // features supported by both device & driver

} virtio_dev_t;

#define BUFFER_INFO_F_WRITE (1 << 0) // Buffer is writable
#define BUFFER_INFO_F_COPY  (1 << 1) // Kernel must copy this buffer

typedef struct Buffer_info
{
    uint64_t addr;
    uint32_t len;
    uint16_t flags;

} buffer_info_t;

uint8_t  virtio_read8 (const virtio_dev_t* virtio_dev, uint32_t offs);
uint16_t virtio_read16(const virtio_dev_t* virtio_dev, uint32_t offs);
uint32_t virtio_read32(const virtio_dev_t* virtio_dev, uint32_t offs);

void virtio_write8 (const virtio_dev_t* virtio_dev, uint32_t offs, uint8_t  value);
void virtio_write16(const virtio_dev_t* virtio_dev, uint32_t offs, uint16_t value);
void virtio_write32(const virtio_dev_t* virtio_dev, uint32_t offs, uint32_t value);

void virtio_set_dev_status_flag  (const virtio_dev_t* virtio_dev, uint8_t flag);
bool virtio_check_dev_status_flag(const virtio_dev_t* virtio_dev, uint8_t flag);

int virtio_setup_virtqueue(virtqueue_t* virtqueue, uint16_t size, size_t chunk_size);
int virtio_setup_vring(vring_t* vring, uint16_t size);

void dump_virtqueue(const virtqueue_t* virtqueue);

int virtio_snd_buffers(virtio_dev_t* virtio_dev, unsigned qind, const buffer_info_t* buffer_info, unsigned buffers_num);

#define ALIGN(x, qalign) (((x) + (qalign - 1)) & (~(qalign - 1))) 
#define QALIGN PAGE_SIZE

static inline size_t vring_calc_size(uint16_t size)
{
    return ALIGN(sizeof(vring_desc_t) * size + sizeof(uint16_t) * (2 + size), QALIGN)
         + ALIGN(sizeof(uint16_t) * 2 + sizeof(vring_used_elem_t) * size, QALIGN);
}

static inline void virtq_used_notif_disable(virtqueue_t* virtqueue)
{
    assert(virtqueue);
    virtqueue->vring.avail->flags = 1; // spec: If flags is 1, the device SHOULD NOT send a notification
}

static inline void virtq_used_notif_enable (virtqueue_t* virtqueue)
{
    assert(virtqueue);
    virtqueue->vring.avail->flags = 0; // spec: If flags is 0, the device MUST send a notification
}

// 1 - should not send; 0 - send avail notification
static inline bool virtq_avail_notif_suppressed_check(const virtqueue_t* virtqueue)
{
    assert(virtqueue);
    return virtqueue->vring.used->flags;
}

#endif /* !JOS_KERN_VIRTIO_H */
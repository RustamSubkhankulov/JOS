#include <inc/assert.h>
#include <inc/error.h>
#include <inc/x86.h>

#include <kern/traceopt.h>
#include <kern/virtio.h>
#include <kern/pmap.h>
#include <inc/string.h>

static size_t vring_calc_size(uint16_t size);
static void*  vring_alloc_mem(size_t mem_size);
static uint16_t virtio_descr_add_buffers(virtqueue_t* virtqueue, const buffer_info_t* buffer_info, 
                                                                             unsigned buffers_num);

static void dump_virtqueue(const virtqueue_t* virtqueue);

void virtio_snd_buffers(virtio_dev_t* virtio_dev, unsigned qind, const buffer_info_t* buffer_info, 
                                                                             unsigned buffers_num)
{
    assert(virtio_dev);
    assert(buffer_info);
    assert(qind < virtio_dev->queues_n);

    if (buffers_num == 0)
        return;

    virtqueue_t* virtqueue = virtio_dev->queues + qind;
    uint16_t chain_head = virtio_descr_add_buffers(virtqueue, buffer_info, buffers_num);
    
    uint16_t avail_ind = virtqueue->vring.avail->idx;
    virtqueue->vring.avail->ring[avail_ind] = chain_head;

    mfence();
    virtqueue->vring.avail->idx = (avail_ind + 1) % virtqueue->vring.num;
    mfence();

    if (virtio)
}

static void dump_virtqueue(const virtqueue_t* virtqueue)
{
    assert(virtqueue);
    return; // TODO
}

static uint16_t virtio_descr_add_buffers(virtqueue_t* virtqueue, const buffer_info_t* buffer_info, 
                                                                             unsigned buffers_num)
{
    assert(buffer_info);
    assert(virtqueue);

    uint16_t descr_free = virtqueue->free_index;
    uint16_t ret_value  = descr_free; 

    uint16_t next_descr_free = 0;

    for (unsigned iter = 0; iter < buffers_num; iter++)
    {
        next_descr_free = (descr_free + 1) % virtqueue->vring.num;
        const buffer_info_t* cur_buffer = buffer_info + iter;

        virtqueue->vring.desc[descr_free].len   = cur_buffer->len;
        virtqueue->vring.desc[descr_free].addr  = cur_buffer->addr;
        virtqueue->vring.desc[descr_free].flags = cur_buffer->flags;
        
        if (iter != buffers_num - 1)
            virtqueue->vring.desc[descr_free].flags |= VIRTQ_DESC_F_NEXT;

        uint16_t buffer_next = (iter == buffers_num - 1)? 0: next_descr_free;
        virtqueue->vring.desc[descr_free].next = buffer_next;

        descr_free = next_descr_free;
    }

    virtqueue->free_index = descr_free;
    return ret_value;
}

static void* vring_alloc_mem(size_t mem_size)
{
    int class = 0; 

    for (; class < MAX_CLASS; class++)
    {
        if (mem_size <= CLASS_SIZE(class))
            break;
    }

    struct Page* allocated_page = alloc_page(class, 0);
    if (allocated_page == NULL)
        return NULL;

    return (void*) ((uint64_t) page2pa(allocated_page));
}

int virtio_setup_virtqueue(virtqueue_t* virtqueue, uint16_t size)
{
    assert(virtqueue);

    int err = virtio_setup_vring(&(virtqueue->vring), size);
    if (err != 0) return err;

    virtqueue->free_index = 0;
    virtqueue->last_used  = 0;
    virtqueue->last_avail = 0;

    return 0;
}

int virtio_setup_vring(vring_t* vring, uint16_t size)
{
    assert(vring);

    size_t mem_size = vring_calc_size(size);

    void* memory = vring_alloc_mem(mem_size);
    if (memory == NULL)
        return -1;

    if ((uint64_t) memory > 4 * GB)
    {
        if (trace_net)
            cprintf("Physical addresses higher than 4 are not supported. ");

        return -1;
    }

    memset(memory, 0x0, mem_size);
    vring->num = size;

    vring->desc  = (vring_desc_t*) memory;
    vring->avail = (vring_avail_t*) ((unsigned char*) memory + sizeof(vring_desc_t) * size);
    vring->used  = (vring_used_t*) &((unsigned char*) memory)[ALIGN(sizeof(vring_desc_t) * size + sizeof(uint16_t) * (2 + size), QALIGN)];

    return 0;
}

uint8_t virtio_read8(const virtio_dev_t* virtio_dev, uint32_t offs)
{
    assert(virtio_dev);

    uint8_t first = 0;
    uint8_t secnd = 0;

    do
    {

        first = inb((virtio_dev->pci_dev_general.BAR0 & IOS_BAR_BASE_ADDR) + offs);
        secnd = inb((virtio_dev->pci_dev_general.BAR0 & IOS_BAR_BASE_ADDR) + offs);

    } while(first != secnd);

    return first;
}

uint16_t virtio_read16(const virtio_dev_t* virtio_dev, uint32_t offs)
{
    assert(virtio_dev);

    uint16_t first = 0;
    uint16_t secnd = 0;

    do
    {

        first = inw((virtio_dev->pci_dev_general.BAR0 & IOS_BAR_BASE_ADDR) + offs);
        secnd = inw((virtio_dev->pci_dev_general.BAR0 & IOS_BAR_BASE_ADDR) + offs);

    } while(first != secnd);

    return first;
}

uint32_t virtio_read32(const virtio_dev_t* virtio_dev, uint32_t offs)
{
    assert(virtio_dev);

    uint32_t first = 0;
    uint32_t secnd = 0;

    do
    {

        first = inl((virtio_dev->pci_dev_general.BAR0 & IOS_BAR_BASE_ADDR) + offs);
        secnd = inl((virtio_dev->pci_dev_general.BAR0 & IOS_BAR_BASE_ADDR) + offs);

    } while(first != secnd);

    return first;
}

void virtio_write8(const virtio_dev_t* virtio_dev, uint32_t offs, uint8_t value)
{
    assert(virtio_dev);
    
    outb((virtio_dev->pci_dev_general.BAR0 & IOS_BAR_BASE_ADDR) + offs, value);
    return;
}

void virtio_write16(const virtio_dev_t* virtio_dev, uint32_t offs, uint16_t value)
{
    assert(virtio_dev); 

    outw((virtio_dev->pci_dev_general.BAR0 & IOS_BAR_BASE_ADDR) + offs, value);
    return;
}

void virtio_write32(const virtio_dev_t* virtio_dev, uint32_t offs, uint32_t value)
{
    assert(virtio_dev); 

    outl((virtio_dev->pci_dev_general.BAR0 & IOS_BAR_BASE_ADDR) + offs, value);
    return;
}

void virtio_set_dev_status_flag(const virtio_dev_t* virtio_dev, uint8_t flag)
{
    assert(virtio_dev); 

    uint8_t device_status = virtio_read8(virtio_dev, VIRTIO_PCI_DEVICE_STATUS);
    virtio_write8(virtio_dev, VIRTIO_PCI_DEVICE_STATUS, device_status | flag);

    return;
}

bool virtio_check_dev_status_flag(const virtio_dev_t* virtio_dev, uint8_t flag)
{
    assert(virtio_dev); 

    uint8_t device_status = virtio_read8(virtio_dev, VIRTIO_PCI_DEVICE_STATUS);
    return (device_status & flag);
}

bool virtio_check_dev_feature(const virtio_dev_t* virtio_dev, uint8_t feature)
{
    assert(virtio_dev); 

    uint32_t device_features = virtio_read32(virtio_dev, VIRTIO_PCI_DEVICE_FEATURES);
    return (device_features & feature);
}

void virtio_set_guest_feature(virtio_dev_t* virtio_dev, uint8_t feature)
{
    assert(virtio_dev); 

    uint32_t guest_features = virtio_read32(virtio_dev, VIRTIO_PCI_GUEST_FEATURES);
    virtio_write32(virtio_dev, VIRTIO_PCI_GUEST_FEATURES, guest_features | feature);

    return;
}
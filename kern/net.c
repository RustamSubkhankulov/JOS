#include <inc/assert.h>
#include <inc/error.h>
#include <inc/uefi.h>
#include <inc/x86.h>
#include <inc/string.h>

#include <kern/traceopt.h>
#include <kern/net.h>
#include <kern/pmap.h>

static int virtio_nic_dev_init        (virtio_nic_dev_t* virtio_nic_dev);
static int virtio_nic_dev_reset       (virtio_nic_dev_t* virtio_nic_dev);
static int virtio_nic_dev_neg_features(virtio_nic_dev_t* virtio_nic_dev);

static int virtio_nic_setup           (virtio_nic_dev_t* virtio_nic_dev);
static int virtio_nic_setup_virtqueues(virtio_nic_dev_t* virtio_nic_dev);
// static int virtio_fill_rcvq           (virtio_nic_dev_t* virtio_nic_dev)
static void virtio_read_mac_addr      (virtio_nic_dev_t* virtio_nic_dev);

static int virtio_nic_alloc_virtqueues(virtio_nic_dev_t* virtio_nic_dev);

static int virtio_nic_check_and_reset (virtio_nic_dev_t* virtio_nic_dev);
void init_net(void)
{
    if (trace_net)
        cprintf("Net initiaization started. \n");

    virtio_nic_dev_t virtio_nic_dev = { 0 };

    int found = pci_dev_find((pci_dev_t*) (&virtio_nic_dev), 
                                            Virtio_nic_pci_class, 
                                            Virtio_nic_pci_subclass,
                                            Virtio_pci_vendor_id);
    if (found == -1)
        panic("Virtio NIC was not found. \n");
    
    int err = pci_dev_general_read_header((pci_dev_general_t*) (&virtio_nic_dev));
    if (err == -1)
        panic("Virtio NIC incorrect bus, dev & func parameters. \n");

    if (trace_net)
        dump_pci_dev_general((pci_dev_general_t*) (&virtio_nic_dev));


    /* Transitional device requirements */
    if (virtio_nic_dev.virtio_dev.pci_dev_general.pci_dev.device_id   != Virtio_nic_pci_device_id
     || virtio_nic_dev.virtio_dev.pci_dev_general.pci_dev.revision_id != 0
     || virtio_nic_dev.virtio_dev.pci_dev_general.subsystem_dev_id    != Virtio_nic_device_id)
    {
        cprintf("Error: Network card is not transitional device. \n");
        return;
    }

    err = virtio_nic_dev_reset(&virtio_nic_dev);
    if (err != 0) 
        panic("Error occured during VirtIO NIC reset. Pernel kanic. \n");

    err = virtio_nic_dev_init(&virtio_nic_dev);
    if (err != 0)
        panic("Error occured during VirtIO NIC initialization. Pernel kanic. \n");

    cprintf("Net initialization successfully finished. \n");
    return;
}

static int virtio_nic_dev_init(virtio_nic_dev_t* virtio_nic_dev)
{
    assert(virtio_nic_dev);

    int err = 0;

    virtio_set_dev_status_flag((virtio_dev_t*) virtio_nic_dev, VIRTIO_PCI_STATUS_ACKNOWLEDGE);
    virtio_set_dev_status_flag((virtio_dev_t*) virtio_nic_dev, VIRTIO_PCI_STATUS_DRIVER);

    err = virtio_nic_dev_neg_features(virtio_nic_dev);
    if (err != 0) return err;

    err = virtio_nic_setup(virtio_nic_dev); // device specific setup
    if (err != 0) return err;

    virtio_set_dev_status_flag((virtio_dev_t*) virtio_nic_dev, VIRTIO_PCI_STATUS_DRIVER_OK);
    return 0;
}

static int virtio_nic_dev_reset(virtio_nic_dev_t* virtio_nic_dev)
{
    assert(virtio_nic_dev);

    virtio_write8((virtio_dev_t*) virtio_nic_dev, VIRTIO_PCI_DEVICE_STATUS, 0x0);

    uint8_t device_status = 0;

    do
    {
        device_status = virtio_read8((virtio_dev_t*) virtio_nic_dev, VIRTIO_PCI_DEVICE_STATUS);

    } while (device_status != 0);

    return 0;
}

static int virtio_nic_check_and_reset(virtio_nic_dev_t* virtio_nic_dev)
{
    assert(virtio_nic_dev);

    if (!virtio_check_dev_status_flag((virtio_dev_t*) virtio_nic_dev, VIRTIO_PCI_STATUS_DEVICE_NEEDS_RESET))
        return 0; // no reset 

    int err = 0;

    err = virtio_nic_dev_reset(virtio_nic_dev);
    if (err != 0) return err;

    err = virtio_nic_dev_init(virtio_nic_dev);
    if (err != 0) return err;

    return 1; // reset happened
}

static int virtio_nic_dev_neg_features(virtio_nic_dev_t* virtio_nic_dev)
{
    assert(virtio_nic_dev);

    uint32_t supported_f = virtio_read32((virtio_dev_t*) virtio_nic_dev, VIRTIO_PCI_DEVICE_FEATURES);
    
    assert(supported_f & VIRTIO_F_RING_EVENT_IDX);

    if (trace_net)
        cprintf("Device features: 0x%x \n", supported_f);

    uint32_t requested_f = VIRTIO_NET_F_MAC; // (VIRTIO_F_VERSION_1 is not set - using Legacy Interface)

    if (trace_net)
        cprintf("Guest features (requested): 0x%x \n", requested_f);

    if (requested_f != 0 && !(supported_f & requested_f))
    {
        if (trace_net)
            cprintf("Device does not support requested features. \n");

        virtio_set_dev_status_flag((virtio_dev_t*) virtio_nic_dev, VIRTIO_PCI_STATUS_FAILED);
        return -1;
    }

    virtio_nic_dev->virtio_dev.features = supported_f & requested_f;

    virtio_write32((virtio_dev_t*) virtio_nic_dev, VIRTIO_PCI_GUEST_FEATURES, requested_f);
    
    /* According to specification, these steps below must be omitted.
       Although, I will leave it here in case we need them later for some reason .*/

    // virtio_set_dev_status_flag((pci_dev_general_t*) virtio_nic_dev, VIRTIO_PCI_STATUS_FEATURES_OK);
    // bool features_ok = virtio_check_dev_status_flag((pci_dev_general_t*) virtio_nic_dev, VIRTIO_PCI_STATUS_FEATURES_OK);
    // if (!features_ok)
    // {
    //     if (trace_net)
    //         cprintf("Device does not support requested subset of features and thus it is unusuble. \n");
    //
    //     virtio_set_dev_status_flag((pci_dev_general_t*) virtio_nic_dev, VIRTIO_PCI_STATUS_FAILED);
    //     return -1;
    // }

    return 0;
}

static int virtio_nic_alloc_virtqueues(virtio_nic_dev_t* virtio_nic_dev)
{
    assert(virtio_nic_dev); 

    size_t size = QUEUE_NUM * sizeof(virtqueue_t);
    int class = 0;

    for (; class < MAX_CLASS; class++)
    {
        if (size < CLASS_SIZE(class))
            break;
    }

    struct Page* allocated = alloc_page(class, 0);
    if (allocated == NULL)
        return -1;

    void* memory = (void*) ((uint64_t) page2pa(allocated));
    memset(memory, 0, size);
    virtio_nic_dev->virtio_dev.queues = (virtqueue_t*) memory;

    return 0;
}

static int virtio_nic_setup_virtqueues(virtio_nic_dev_t* virtio_nic_dev)
{
    assert(virtio_nic_dev); 

    int err = virtio_nic_alloc_virtqueues(virtio_nic_dev);
    if (err != 0)
    {
        if (trace_net)
            cprintf("Failed to alloc memory for virtqueues. \n");
        
        virtio_set_dev_status_flag((virtio_dev_t*) virtio_nic_dev, VIRTIO_PCI_STATUS_FAILED);
        return -1;
    }

    for (unsigned iter = 0; iter < QUEUE_NUM; iter++)
    {
        virtio_write16((virtio_dev_t*) virtio_nic_dev, VIRTIO_PCI_QUEUE_SELECT, iter);
        uint16_t size = virtio_read16((virtio_dev_t*) virtio_nic_dev, VIRTIO_PCI_QUEUE_SIZE);

        if (trace_net)
            cprintf("QueueN%d size: 0x%x \n", iter, size);

        if (size == 0)
        {
            if (trace_net)
                cprintf("All requested queues must exist. \n");

            virtio_set_dev_status_flag((virtio_dev_t*) virtio_nic_dev, VIRTIO_PCI_STATUS_FAILED);
            return -1;
        }

        err = virtio_setup_virtqueue(virtio_nic_dev->virtio_dev.queues + iter, size);
        if (err != 0)
        {
            if (trace_net)
                cprintf("Failed to allocate memory for virtqueue. \n");

            virtio_set_dev_status_flag((virtio_dev_t*) virtio_nic_dev, VIRTIO_PCI_STATUS_FAILED);
            return -1;
        }

        virtio_write16((virtio_dev_t*) virtio_nic_dev, VIRTIO_PCI_QUEUE_SELECT, iter);
        virtio_write32((virtio_dev_t*) virtio_nic_dev, VIRTIO_PCI_QUEUE_ADDR, 
                           (uint64_t) (virtio_nic_dev->virtio_dev.queues + iter)->vring.desc / PAGE_SIZE);
    
        if (trace_net)
            cprintf("Address of memory allocated for QUEUEN%d: 0x%lx IN PAGES: 0x%llx \n", iter,
                                                            (uint64_t) (virtio_nic_dev->virtio_dev.queues + iter)->vring.desc, 
                                                            (uint64_t) (virtio_nic_dev->virtio_dev.queues + iter)->vring.desc / PAGE_SIZE);
    }

    virtio_nic_dev->virtio_dev.queues_n = QUEUE_NUM;

    return 0;
}

// static int virtio_fill_rcvq(virtio_nic_dev_t* virtio_nic_dev)
// {
//     assert(virtio_nic_dev);
//     return 0; // TODO
// }

static void virtio_read_mac_addr(virtio_nic_dev_t* virtio_nic_dev)
{
    assert(virtio_nic_dev);

    for (unsigned iter = 0; iter < MAC_ADDR_NUM; iter++)
    {
        virtio_nic_dev->MAC[iter] = virtio_read8((virtio_dev_t*) virtio_nic_dev, VIRTIO_PCI_NET_MAC1 + iter); 
    }

    if (trace_net)
        cprintf("MAC address: %d.%d.%d.%d.%d.%d \n", virtio_nic_dev->MAC[0], 
                                                     virtio_nic_dev->MAC[1],
                                                     virtio_nic_dev->MAC[2],
                                                     virtio_nic_dev->MAC[3],
                                                     virtio_nic_dev->MAC[4],
                                                     virtio_nic_dev->MAC[5]);

    return;
}

static int virtio_nic_setup(virtio_nic_dev_t* virtio_nic_dev)
{
    assert(virtio_nic_dev);

    int err = 0;

    virtio_read_mac_addr(virtio_nic_dev);

    err = virtio_nic_setup_virtqueues(virtio_nic_dev);
    if (err != 0) return err;

    // err = virtio_fill_rcvq(virtio_nic_dev);
    // if (err != 0) return err;

    return 0;
}
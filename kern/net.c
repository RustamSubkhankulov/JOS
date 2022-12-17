#include <inc/assert.h>
#include <inc/error.h>
#include <inc/uefi.h>
#include <inc/x86.h>
#include <inc/string.h>

#include <kern/traceopt.h>
#include <kern/net.h>
#include <kern/pmap.h>
#include <kern/picirq.h>

// TODO: remove
#include <kern/udp.h>

static virtio_nic_dev_t* Virtio_nic_device = NULL;

static int virtio_nic_dev_init        (virtio_nic_dev_t* virtio_nic_dev);
static int virtio_nic_dev_reset       (virtio_nic_dev_t* virtio_nic_dev);
static int virtio_nic_dev_neg_features(virtio_nic_dev_t* virtio_nic_dev);

static int virtio_nic_setup           (virtio_nic_dev_t* virtio_nic_dev);
static int virtio_nic_setup_virtqueues(virtio_nic_dev_t* virtio_nic_dev);
static int virtio_fill_rcvq           (virtio_nic_dev_t* virtio_nic_dev);
static void virtio_read_mac_addr      (virtio_nic_dev_t* virtio_nic_dev);

static int virtio_nic_alloc_virtqueues(virtio_nic_dev_t* virtio_nic_dev);

static int virtio_nic_check_and_reset (virtio_nic_dev_t* virtio_nic_dev);

static void clear_snd_buffers         (virtio_nic_dev_t* virtio_nic_dev);
static void reclaim_rcv_buffers       (virtio_nic_dev_t* virtio_nic_dev);

void init_net(void)
{
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

    // TEST

    // Test IPv4 & UDP wrappers
    ip_pkt_t pkt = {};

    ip_port_t src = { .addr.word = 0xC00002EB, 1234 };
    ip_port_t dst = { .addr.word = -1, 1337 };

    char tmp_data[] = "DIO_JOJO\n";

    make_udp_pkt(src, dst, tmp_data, sizeof(tmp_data), &pkt);

    dump_pkt(&pkt);

    // char buf1[12] = "Hello world";
    buffer_info_t bufi1 = {.addr = (uint64_t)&pkt, .flags = BUFFER_INFO_F_COPY, .len = 100};
    err = virtio_nic_snd_buffer(&virtio_nic_dev, &bufi1);
    if (err != 0)
    {
        cprintf("Test virtio_nic_snd_buffer failed. \n");
    }

    // char buf2[10] = "Goodb ye.";
    // buffer_info_t bufi2 = {.addr = (uint64_t) buf2, .flags = BUFFER_INFO_F_COPY, .len = 9};
    // err = virtio_nic_snd_buffer(&virtio_nic_dev, &bufi2);
    // if (err != 0)
    // {
    //     cprintf("Test virtio_nic_snd_buffer failed. \n");
    // }

    // dump_virtqueue(&virtio_nic_dev.virtio_dev.queues[SNDQ]);
    // dump_virtqueue(&virtio_nic_dev.virtio_dev.queues[RCVQ]);

    return;
}

int virtio_nic_rcv_buffer(virtio_nic_dev_t* virtio_nic_dev, buffer_info_t* buffer_info)
{
    assert(virtio_nic_dev);
    assert(buffer_info);
    return 0; // TODO
}

int virtio_nic_snd_buffer(virtio_nic_dev_t* virtio_nic_dev, const buffer_info_t* buffer_info)
{
    assert(virtio_nic_dev);
    assert(buffer_info); // TODO max size

    clear_snd_buffers(virtio_nic_dev); // TODO: temporary here, until usage of nic in userspace is available

    if (buffer_info->len > SND_MAX_SIZE)
    {
        if (trace_net)
            cprintf("Size of sending buffer is more than possible maximum. Failed. \n");
        return -1;
    }

    virtio_net_hdr_t net_hdr = { 0 };
    // TODO work with header
    net_hdr.flags       = VIRTIO_NET_HDR_F_NEEDS_CSUM;
    net_hdr.gso_type    = VIRTIO_NET_HDR_GSO_NONE;
    net_hdr.csum_start  = 0;
    net_hdr.csum_offset = buffer_info->len;
    net_hdr.hdr_len     = 0; 
    net_hdr.gso_size    = 0; 
    // net_hdr.num_buffers = 0;

    buffer_info_t to_send[2] = { 0 };

    to_send[0].flags = BUFFER_INFO_F_COPY;
    to_send[0].addr  = (uint64_t) &net_hdr;
    to_send[0].len   = sizeof(net_hdr);

    to_send[1].flags = buffer_info->flags;
    to_send[1].addr  = buffer_info->addr;
    to_send[1].len   = buffer_info->len;

    return virtio_snd_buffers((virtio_dev_t*) virtio_nic_dev, SNDQ, to_send, 2);
}

void net_irq_handler(void)
{
    if (trace_net)
        cprintf("NIC HANDLER HELLO HELLO BITCHES! \n");

    uint8_t isr_status = virtio_read8((virtio_dev_t*) Virtio_nic_device, VIRTIO_PCI_ISR_STATUS);
    if (isr_status & ISR_STATUS_QUEUE_INT)
    {
        clear_snd_buffers(Virtio_nic_device);
    }

    pic_send_eoi(IRQ_NIC);
    return;
}

static int virtio_nic_dev_init(virtio_nic_dev_t* virtio_nic_dev)
{
    assert(virtio_nic_dev);

    if (trace_net)
        cprintf("VirtIO general initialization started. \n");

    int err = 0;

    virtio_set_dev_status_flag((virtio_dev_t*) virtio_nic_dev, VIRTIO_PCI_STATUS_ACKNOWLEDGE);
    virtio_set_dev_status_flag((virtio_dev_t*) virtio_nic_dev, VIRTIO_PCI_STATUS_DRIVER);

    err = virtio_nic_dev_neg_features(virtio_nic_dev);
    if (err != 0) return err;

    err = virtio_nic_setup(virtio_nic_dev); // device specific setup
    if (err != 0) return err;

    if (virtio_nic_dev->virtio_dev.pci_dev_general.interrupt_line != IRQ_NIC)
    {
        cprintf("VirtIO network card cupported only on IRQ 11 \n");

        virtio_set_dev_status_flag((virtio_dev_t*) virtio_nic_dev, VIRTIO_PCI_STATUS_FAILED);
        return -1;
    }

    pic_irq_unmask(IRQ_NIC);

    if (trace_net)
        cprintf("IRQ_NET unmasked. \n");

    if (trace_net)
        cprintf("VirtIO nic initialization completed. Sending DRIVER_OK to device. \n");

    Virtio_nic_device = virtio_nic_dev; // assign global variable so handler could use device struct 
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

    if (trace_net)
        cprintf("VirtIO nic device has been reset. \n");

    return 0;
}

static int virtio_nic_check_and_reset(virtio_nic_dev_t* virtio_nic_dev)
{
    assert(virtio_nic_dev);

    if (trace_net)
        cprintf("Performing VirtIO device status check. \n");

    if (!virtio_check_dev_status_flag((virtio_dev_t*) virtio_nic_dev, VIRTIO_PCI_STATUS_DEVICE_NEEDS_RESET))
    {
        if (trace_net)
            cprintf("VirtIO nic is ok, no need to reset. \n");

        return 0; // no reset 
    }

    if (trace_net)
        cprintf("VirtIO nic is not OK, resetting. \n");

    int err = virtio_nic_dev_reset(virtio_nic_dev);
    if (err != 0) return err;

    return 1; // reset happened
}

static int virtio_nic_dev_neg_features(virtio_nic_dev_t* virtio_nic_dev)
{
    assert(virtio_nic_dev);

    if (trace_net)
        cprintf("VirtIO nic: performing features negotiating. \n");

    uint32_t supported_f = virtio_read32((virtio_dev_t*) virtio_nic_dev, VIRTIO_PCI_DEVICE_FEATURES);
    
    assert(supported_f & VIRTIO_F_RING_EVENT_IDX);

    if (trace_net)
        cprintf("Device features: 0x%x \n", supported_f);

    uint32_t requested_f = VIRTIO_NET_F_MAC
                         | VIRTIO_NET_F_CSUM;
                        //  | VIRTIO_NET_F_HOST_UFO   // These features may be used in future.  
                        //  | VIRTIO_NET_F_GUEST_CSUM
                        //  | VIRTIO_NET_F_GUEST_UFO; // (VIRTIO_F_VERSION_1 is not set - using Legacy Interface)

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

    if (trace_net)
        cprintf("VirtIO nic: performing virtqueues setup. \n");

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

        err = virtio_setup_virtqueue(virtio_nic_dev->virtio_dev.queues + iter, size, SND_MAX_SIZE);
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

    virtq_used_notif_enable(&(virtio_nic_dev->virtio_dev.queues[SNDQ]));
    virtq_used_notif_disable(&(virtio_nic_dev->virtio_dev.queues[RCVQ])); // TODO: temporary disable since receive is not supported yet
                                                                          // Note: disabling ism't strict for device, it can still deliver notification

    if (trace_net)
        cprintf("VirtIO nic virtqueues setup completed. \n");

    return 0;
}

static int virtio_fill_rcvq(virtio_nic_dev_t* virtio_nic_dev)
{
    assert(virtio_nic_dev);

    char rcv_buffer[RCV_MAX_SIZE] = { 0 };
    buffer_info_t buffer_info = {.addr  = (uint64_t) rcv_buffer, 
                                 .flags = BUFFER_INFO_F_COPY | BUFFER_INFO_F_WRITE, 
                                 .len   = RCV_MAX_SIZE};

    for (unsigned iter = 0; iter < virtio_nic_dev->virtio_dev.queues[RCVQ].vring.num; iter++)
    {
        int err = virtio_snd_buffers((virtio_dev_t*) virtio_nic_dev, RCVQ, &buffer_info, 1);
        if (err != 0)
        {
            cprintf("Failed to add free buffers to desct table in RSVQ. \n");
            return -1;
        }
    }

    return 0;
}

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

    if (trace_net)
        cprintf("VirtIO nic: performing device specific initialization. \n");

    int err = 0;
    virtio_read_mac_addr(virtio_nic_dev);

    err = virtio_nic_setup_virtqueues(virtio_nic_dev);
    if (err != 0) return err;

    err = virtio_fill_rcvq(virtio_nic_dev);
    if (err != 0) return err;

    if (trace_net)
        cprintf("VirtIO nic: device specific initialization completed. \n");

    return 0;
}

static void clear_snd_buffers(virtio_nic_dev_t* virtio_nic_dev)
{
    assert(virtio_nic_dev);
    virtqueue_t* sndq = &(virtio_nic_dev->virtio_dev.queues[SNDQ]);

    if (trace_net)
        cprintf("Reclaiming used buffers in sndq. Num_free mow is %d. \n", sndq->num_free);

    while (sndq->last_used != sndq->vring.used->idx)
    {
        uint16_t index = sndq->last_used % sndq->vring.num;

        vring_used_elem_t* used_elem = &(sndq->vring.used->ring[index]);
        uint16_t desc_idx = used_elem->index;

        uint16_t chain_len = 1;
        while (sndq->vring.desc[desc_idx].flags & VIRTQ_DESC_F_NEXT)
        {
            chain_len += 1;
            desc_idx = sndq->vring.desc[desc_idx].next;
        }

        sndq->num_free  += chain_len;
        sndq->last_used += 1;
    }

    if (trace_net)
        cprintf("Cleared used buffers. Num_free now is %d \n", sndq->num_free);

    return;
}

static void reclaim_rcv_buffers(virtio_nic_dev_t* virtio_nic_dev)
{
    assert(virtio_nic_dev);
    virtqueue_t* rcvq = &(virtio_nic_dev->virtio_dev.queues[RCVQ]);

    if (trace_net)
        cprintf("Reclaiming used buffers in rcvq. Num_free mow is %d. \n", rcvq->num_free);

    while (rcvq->last_used != rcvq->vring.used->idx)
    {
        uint16_t index = rcvq->last_used % rcvq->vring.num;

        vring_used_elem_t* used_elem = &(rcvq->vring.used->ring[index]);
        uint16_t desc_idx = used_elem->index;

        
        buffer_info_t buffer_info = {.addr  =  rcvq->vring.desc[desc_idx].addr,
                                     .flags = (rcvq->vring.desc[desc_idx].flags & VIRTQ_DESC_F_WRITE)? BUFFER_INFO_F_WRITE: 0, 
                                     .len   =  rcvq->vring.desc[desc_idx].len};
        rcvq->num_free += 1;
        int err = virtio_snd_buffers((virtio_dev_t*) virtio_nic_dev, RCVQ, &buffer_info, 1);
        if (err != 0)
            panic("failed to reclaim buffer in rcvq. \n");

        while (rcvq->vring.desc[desc_idx].flags & VIRTQ_DESC_F_NEXT)
        {
            desc_idx = rcvq->vring.desc[desc_idx].next;

            buffer_info.addr  =  rcvq->vring.desc[desc_idx].addr;
            buffer_info.flags = (rcvq->vring.desc[desc_idx].flags & VIRTQ_DESC_F_WRITE)? BUFFER_INFO_F_WRITE: 0;
            buffer_info.len   =  rcvq->vring.desc[desc_idx].len;
            rcvq->num_free += 1;

            err = virtio_snd_buffers((virtio_dev_t*) virtio_nic_dev, RCVQ, &buffer_info, 1);
            if (err != 0)
                panic("failed to reclaim buffer in rcvq. \n");
        }

        rcvq->last_used += 1;
    }

    if (trace_net)
        cprintf("Reclaimed used buffers. Num_free now is %d \n", rcvq->num_free);

    return;
}
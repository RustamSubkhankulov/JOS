#include <inc/assert.h>
#include <inc/error.h>
#include <inc/uefi.h>
#include <inc/x86.h>

#include <kern/traceopt.h>
#include <kern/net.h>

static int virtio_nic_dev_init        (virtio_nic_dev_t* virtio_nic_dev);
static int virtio_nic_dev_reset       (virtio_nic_dev_t* virtio_nic_dev);
static int virtio_nic_dev_neg_features(virtio_nic_dev_t* virtio_nic_dev);

static int virtio_nic_setup           (virtio_nic_dev_t* virtio_nic_dev);

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

    assert((virtio_nic_dev.pci_dev_general.pci_dev.device_id == Virtio_nic_pci_device_id)
        || (virtio_nic_dev.pci_dev_general.pci_dev.device_id == Virtio_nic_device_id + PCI_DEVICE_ID_CALC_OFFSET));

    err = virtio_nic_dev_reset(&virtio_nic_dev);
    if (err != 0) 
        panic("Error occured during VirtIO NIC reset. Pernel kanic. \n");

    err = virtio_nic_dev_init(&virtio_nic_dev);
    if (err != 0)
        panic("Error occured during VirtIO NIC initialization. Pernel kanic. \n");

    if (trace_net)
        cprintf("Net initialization successfully finished. \n");

    return;
}

static int virtio_nic_dev_init(virtio_nic_dev_t* virtio_nic_dev)
{
    assert(virtio_nic_dev);

    int err = 0;

    virtio_set_dev_status_flag((pci_dev_general_t*) virtio_nic_dev, VIRTIO_PCI_STATUS_ACKNOWLEDGE);
    virtio_set_dev_status_flag((pci_dev_general_t*) virtio_nic_dev, VIRTIO_PCI_STATUS_DRIVER);

    err = virtio_nic_dev_neg_features(virtio_nic_dev);
    if (err != 0) return err;

    err = virtio_nic_setup(virtio_nic_dev); // device specific setup
    if (err != 0) return err;

    virtio_set_dev_status_flag((pci_dev_general_t*) virtio_nic_dev, VIRTIO_PCI_STATUS_DRIVER_OK);
    return 0;
}

static int virtio_nic_dev_reset(virtio_nic_dev_t* virtio_nic_dev)
{
    assert(virtio_nic_dev);
    cprintf("Don't know yet how to  reset VirtIO device \n"); // TODO
    return 0;
}

static int virtio_nic_check_and_reset(virtio_nic_dev_t* virtio_nic_dev)
{
    assert(virtio_nic_dev);

    if (!virtio_check_dev_status_flag((pci_dev_general_t*) virtio_nic_dev, VIRTIO_PCI_STATUS_DEVICE_NEEDS_RESET))
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

    // negotiate TODO

    virtio_set_dev_status_flag((pci_dev_general_t*) virtio_nic_dev, VIRTIO_PCI_STATUS_FEATURES_OK);

    bool features_supported = virtio_check_dev_status_flag((pci_dev_general_t*) virtio_nic_dev, VIRTIO_PCI_STATUS_FEATURES_OK);
    if (features_supported == false)
    {
        if (trace_net)
            cprintf("Device does not support our subset of features and the device is unusable. \n");

        return -1;
    }

    return 0;
}

static int virtio_nic_setup(virtio_nic_dev_t* virtio_nic_dev)
{
    assert(virtio_nic_dev); // TODO
    return 0;
}
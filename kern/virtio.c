#include <inc/assert.h>
#include <inc/error.h>
#include <inc/x86.h>

#include <kern/traceopt.h>
#include <kern/virtio.h>

uint32_t virtio_get_reg(const pci_dev_general_t* virtio_nic_dev, uint32_t offs)
{
    assert(virtio_nic_dev); // TODO
    return 0;
}

void virtio_set_dev_status_flag(const pci_dev_general_t* virtio_nic_dev, uint8_t flag)
{
    assert(virtio_nic_dev); // TODO
    return;
}

bool virtio_check_dev_status_flag(const pci_dev_general_t* virtio_nic_dev, uint8_t flag)
{
    assert(virtio_nic_dev);
    return false; // TODO
}

bool virtio_check_dev_feature(const pci_dev_general_t* virtio_nic_dev, uint8_t feature)
{
    assert(virtio_nic_dev);
    return false; // TODO
}

void virtio_set_guest_feature(pci_dev_general_t* virtio_nic_dev, uint8_t feature)
{
    assert(virtio_nic_dev); // TODO
    return;
}
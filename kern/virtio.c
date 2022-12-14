#include <inc/assert.h>
#include <inc/error.h>
#include <inc/x86.h>

#include <kern/traceopt.h>
#include <kern/virtio.h>

uint8_t virtio_read8(const pci_dev_general_t* virtio_nic_dev, uint32_t offs)
{
    assert(virtio_nic_dev);

    uint8_t first = 0;
    uint8_t secnd = 0;

    do
    {

        first = inb((virtio_nic_dev->BAR0 & IOS_BAR_BASE_ADDR) + offs);
        secnd = inb((virtio_nic_dev->BAR0 & IOS_BAR_BASE_ADDR) + offs);

    } while(first != secnd);

    return first;
}

uint16_t virtio_read16(const pci_dev_general_t* virtio_nic_dev, uint32_t offs)
{
    assert(virtio_nic_dev);

    uint16_t first = 0;
    uint16_t secnd = 0;

    do
    {

        first = inl((virtio_nic_dev->BAR0 & IOS_BAR_BASE_ADDR) + offs);
        secnd = inl((virtio_nic_dev->BAR0 & IOS_BAR_BASE_ADDR) + offs);

    } while(first != secnd);

    return first;
}

uint32_t virtio_read32(const pci_dev_general_t* virtio_nic_dev, uint32_t offs)
{
    assert(virtio_nic_dev);

    uint32_t first = 0;
    uint32_t secnd = 0;

    do
    {

        first = inw((virtio_nic_dev->BAR0 & IOS_BAR_BASE_ADDR) + offs);
        secnd = inw((virtio_nic_dev->BAR0 & IOS_BAR_BASE_ADDR) + offs);

    } while(first != secnd);

    return first;
}

void virtio_write8(const pci_dev_general_t* virtio_nic_dev, uint32_t offs, uint8_t value)
{
    assert(virtio_nic_dev);
    
    outb((virtio_nic_dev->BAR0 & IOS_BAR_BASE_ADDR) + offs, value);
    return;
}

void virtio_write16(const pci_dev_general_t* virtio_nic_dev, uint32_t offs, uint16_t value)
{
    assert(virtio_nic_dev); 

    outl((virtio_nic_dev->BAR0 & IOS_BAR_BASE_ADDR) + offs, value);
    return;
}

void virtio_write32(const pci_dev_general_t* virtio_nic_dev, uint32_t offs, uint32_t value)
{
    assert(virtio_nic_dev); 

    outw((virtio_nic_dev->BAR0 & IOS_BAR_BASE_ADDR) + offs, value);
    return;
}

void virtio_set_dev_status_flag(const pci_dev_general_t* virtio_nic_dev, uint8_t flag)
{
    assert(virtio_nic_dev); 

    uint8_t device_status = virtio_read8(virtio_nic_dev, VIRTIO_PCI_DEVICE_STATUS);
    virtio_write8(virtio_nic_dev, VIRTIO_PCI_DEVICE_STATUS, device_status | flag);

    return;
}

bool virtio_check_dev_status_flag(const pci_dev_general_t* virtio_nic_dev, uint8_t flag)
{
    assert(virtio_nic_dev); 

    uint8_t device_status = virtio_read8(virtio_nic_dev, VIRTIO_PCI_DEVICE_STATUS);
    return (device_status & flag);
}

bool virtio_check_dev_feature(const pci_dev_general_t* virtio_nic_dev, uint8_t feature)
{
    assert(virtio_nic_dev); 

    uint32_t device_features = virtio_read32(virtio_nic_dev, VIRTIO_PCI_DEVICE_FEATURES);
    return (device_features & feature);
}

void virtio_set_guest_feature(pci_dev_general_t* virtio_nic_dev, uint8_t feature)
{
    assert(virtio_nic_dev); 

    uint32_t guest_features = virtio_read32(virtio_nic_dev, VIRTIO_PCI_GUEST_FEATURES);
    virtio_write32(virtio_nic_dev, VIRTIO_PCI_GUEST_FEATURES, guest_features | feature);

    return;
}
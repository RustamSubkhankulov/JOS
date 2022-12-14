#include <inc/assert.h>
#include <inc/error.h>
#include <inc/x86.h>

#include <kern/traceopt.h>
#include <kern/virtio.h>

void read_virtio_pci_cap(pci_dev_t* pci_dev, virtio_pci_cap_t* virtio_pci_cap, uint8_t offs)
{
    virtio_pci_cap->cap_vndr = pci_config_read8(pci_dev, offs);
    virtio_pci_cap->cap_next = pci_config_read8(pci_dev, offs + 1);
    virtio_pci_cap->cap_len  = pci_config_read8(pci_dev, offs + 2);
    virtio_pci_cap->cfg_type = pci_config_read8(pci_dev, offs + 3);
    virtio_pci_cap->bar      = pci_config_read8(pci_dev, offs + 4);
    
    virtio_pci_cap->offset   = pci_config_read32(pci_dev, offs + 8);
    virtio_pci_cap->length   = pci_config_read32(pci_dev, offs + 12);

    return;
}

uint8_t virtio_read8(const virtio_nic_dev_t* virtio_nic_dev, uint8_t type, uint32_t offs)
{
    assert(virtio_nic_dev);

    uint8_t  bar    = virtio_nic_dev->capabilities[type - 1].bar;

    uint32_t bar_l = virtio_nic_dev->pci_dev_general.BAR[bar];
    uint32_t bar_u = virtio_nic_dev->pci_dev_general.BAR[bar + 1];

    uint64_t addr   = (uint64_t) ((bar_l & 0xFFFFFFF0) + ((uint64_t) (bar_u) << 32));

    cprintf("addr 0x%lx \n", addr);

    uint64_t offset = (uint64_t) virtio_nic_dev->capabilities[type - 1].offset; 

    return *((uint8_t*)(addr + offset + offs));
}

uint16_t virtio_read16(const virtio_nic_dev_t* virtio_nic_dev, uint8_t type, uint32_t offs)
{
    assert(virtio_nic_dev);

    if (virtio_nic_dev->pci_conf_acc_io == false)
        panic("Access to virtio configuration through memory is not implemented yet. \n");

    return 0;
}

uint32_t virtio_read32(const virtio_nic_dev_t* virtio_nic_dev, uint8_t type, uint32_t offs)
{
    assert(virtio_nic_dev);

    if (virtio_nic_dev->pci_conf_acc_io == false)
        panic("Access to virtio configuration through memory is not implemented yet. \n");

    return 0;
}

void virtio_write8(const virtio_nic_dev_t* virtio_nic_dev, uint8_t type, uint32_t offs, uint8_t  value)
{
    assert(virtio_nic_dev);
}

void virtio_write16(const virtio_nic_dev_t* virtio_nic_dev, uint8_t type, uint32_t offs, uint16_t value)
{
    assert(virtio_nic_dev);

    if (virtio_nic_dev->pci_conf_acc_io == false)
        panic("Access to virtio configuration through memory is not implemented yet. \n");

}

void virtio_write32(const virtio_nic_dev_t* virtio_nic_dev, uint8_t type, uint32_t offs, uint32_t value)
{
    assert(virtio_nic_dev);

    if (virtio_nic_dev->pci_conf_acc_io == false)
        panic("Access to virtio configuration through memory is not implemented yet. \n");

}

void virtio_set_dev_status_flag(const pci_dev_general_t* virtio_nic_dev, uint8_t flag)
{
    assert(virtio_nic_dev);  // REWORK

    // uint8_t device_status = virtio_read8(virtio_nic_dev, VIRTIO_PCI_DEVICE_STATUS);
    // virtio_write8(virtio_nic_dev, VIRTIO_PCI_DEVICE_STATUS, device_status | flag); TODO

    return;
}

bool virtio_check_dev_status_flag(const pci_dev_general_t* virtio_nic_dev, uint8_t flag)
{
    assert(virtio_nic_dev);  // REWORK
    return false;

    // uint8_t device_status = virtio_read8(virtio_nic_dev, VIRTIO_PCI_DEVICE_STATUS); TODO
    // return (device_status & flag);
}

bool virtio_check_dev_feature(const pci_dev_general_t* virtio_nic_dev, uint8_t feature)
{
    assert(virtio_nic_dev);  // REWORK
    return false;

    // uint32_t device_features = virtio_read32(virtio_nic_dev, VIRTIO_PCI_DEVICE_FEATURES); TODO
    // return (device_features & feature);
}

void virtio_set_guest_feature(pci_dev_general_t* virtio_nic_dev, uint8_t feature)
{
    assert(virtio_nic_dev);  // REWORK

    // uint32_t guest_features = virtio_read32(virtio_nic_dev, VIRTIO_PCI_GUEST_FEATURES);
    // virtio_write32(virtio_nic_dev, VIRTIO_PCI_GUEST_FEATURES, guest_features | feature); TODO

    return;
}
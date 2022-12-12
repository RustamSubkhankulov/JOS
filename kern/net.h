#ifndef JOS_KERN_NET_H
#define JOS_KERN_NET_H
#ifndef JOS_KERNEL
#error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/pci.h>

const static enum Pci_class            Virtio_pci_class    = PCI_CLASS_NETWORK_CONTROLLER;
const static enum Pci_network_subclass Virtio_pci_subclass = PCI_SUBCLASS_ETHERNET;
const static uint16_t Virtio_pci_vendor_id = 0x1AF4;

void init_net(void);

#endif /* !JOS_KERN_NET_H */
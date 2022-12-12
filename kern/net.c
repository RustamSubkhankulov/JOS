#include <inc/assert.h>
#include <inc/error.h>
#include <inc/uefi.h>
#include <inc/x86.h>

#include <kern/traceopt.h>
#include <kern/net.h>

void init_net(void)
{
    if (trace_net)
        cprintf("Net initiaization started. \n");

    pci_dev_t virtio_dev_pci = { 0 };
    int found = pci_dev_find(&virtio_dev_pci, Virtio_pci_class, 
                                              Virtio_pci_subclass,
                                              Virtio_pci_vendor_id);
    if (found == -1)
        panic("Virtio NIC was not found. \n");
    
    int err = pci_dev_read_header(&virtio_dev_pci);
    if (err == -1)
        panic("Virtio NIC incorrect bus, dev & func parameters. \n");

    // init

    if (trace_net)
        cprintf("Net initialization successfully finished. \n");

    return;
}

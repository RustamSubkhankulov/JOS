#include <inc/assert.h>
#include <inc/error.h>
#include <inc/uefi.h>
#include <inc/x86.h>

#include <kern/traceopt.h>
#include <kern/pci.h>

void init_pci(void)
{
    if (trace_pci)
        cprintf("PCI initialization started. \n");

    int count = enumerate_pci_devices();

    if (trace_pci)
        cprintf("Enumerating PCI devices. Total count=%d \n", count);

    if (trace_pci)
        cprintf("PCI initialization successfully finished. \n");

    return; 
}

/* Enumerating PCI buses */

uint8_t get_header_type (uint8_t bus, uint8_t dev)
{
    pci_dev_t device = { 0 };

    device.bus_number      = bus;
    device.device_number   = dev;
    device.function_number = 0;

    return pci_config_read8(&device, PCI_CONF_SPACE_HEADER_TYPE);
}

bool check_multi_function(uint8_t bus, uint8_t dev)
{
    return (get_header_type(bus, dev) & HEADER_TYPE_REG_MF);
}

uint16_t get_vendor_id(uint8_t bus, uint8_t dev, uint8_t function)
{
    pci_dev_t device = { 0 };

    device.bus_number      = bus;
    device.device_number   = dev;
    device.function_number = function;

    return pci_config_read16(&device, PCI_CONF_SPACE_VENDOR_ID);
}

bool check_pci_device(uint8_t bus, uint8_t device, uint8_t function)
{
    uint16_t vendor_id = get_vendor_id(bus, device, function);
    bool present = (vendor_id != NON_EXISTING_VENDOR_ID);

    if (trace_pci_more && present)
    {
        cprintf("Checking device at: bus=0x%03x device=0x%02x function=0x%x. ", 
                                                        bus, device, function);
        cprintf("Vendor_id=0x%04x. \n", vendor_id);
    }

    return present;
}


// performs Brute Force scan
int enumerate_pci_devices(void) 
{
    int count = 0;

    for (uint16_t bus = 0; bus < MAX_BUS; bus++)
    {
        for (uint8_t dev = 0; dev < MAX_DEV; dev++)
        {
            uint8_t function= 0;

            if (check_pci_device(bus, dev, function) == false)
                continue;

            count++;

            bool is_mf = check_multi_function(bus, dev);

            if (trace_pci_more)
                cprintf("Device(bus=0x%03x device=0x%02x function=0x%x) is MF: %s. \n", 
                                             bus, dev, function, (is_mf)? "yes": "no");                

            if (is_mf)
            {
                for (function = 1; function < MAX_FUN; function++)
                {
                    if (check_pci_device(bus, dev, function) == true)
                        count++;
                }
            }
        }
    }

    return count;
}

/* Read-write functions for PIC configuration space */

uint8_t pci_config_read8(const pci_dev_t* pci_dev, uint8_t offset)
{
    assert(pci_dev);
    return (uint8_t) (pci_config_read32(pci_dev, offset) >> ((offset & 0x3) * 8) & 0xFF);
}


uint16_t pci_config_read16(const pci_dev_t* pci_dev, uint8_t offset)
{
    assert(pci_dev);
    return (uint16_t) (pci_config_read32(pci_dev, offset) >> ((offset & 0x2) * 8) & 0xFFFF);
}

uint32_t pci_config_read32(const pci_dev_t* pci_dev, uint8_t offset)
{
    assert(pci_dev);

    uint32_t addr = PCI_CONF_ADDR_PORT_ENABLE_BIT 
                  | ((uint32_t) pci_dev->bus_number)      << PCI_CONF_ADDR_PORT_BUS_OFFS
                  | ((uint32_t) pci_dev->device_number)   << PCI_CONF_ADDR_PORT_DEV_OFFS
                  | ((uint32_t) pci_dev->function_number) << PCI_CONF_ADDR_PORT_FUN_OFFS
                  | (offset & PCI_REGISTER_OFFS_MASK);

    outl(PCI_CONFIGURATION_ADDR_PORT, addr);
    return inl(PCI_CONFIGURATION_DATA_PORT);
}

void pci_config_write8(const pci_dev_t* pci_dev, uint8_t offset, uint8_t val)
{
    assert(pci_dev);

    uint16_t current = pci_config_read16(pci_dev, offset);
    uint16_t new     = ((uint16_t)val << ((offset & 0x1) * 8)) | (current & (0xFF00 >> (offset & 0x1) * 8));

    pci_config_write16(pci_dev, offset, new);
}

void pci_config_write16(const pci_dev_t* pci_dev, uint8_t offset, uint16_t val)
{
    assert(pci_dev);

    uint32_t current = pci_config_read32(pci_dev, offset);
    uint32_t new     = ((uint32_t)val << ((offset & 0x2) * 8)) | (current & (0xFFFF0000 >> (offset & 0x2) * 8));

    pci_config_write32(pci_dev, offset, new);
}

void pci_config_write32(const pci_dev_t* pci_dev, uint8_t offset, uint32_t val)
{
    assert(pci_dev);

    uint32_t addr = PCI_CONF_ADDR_PORT_ENABLE_BIT 
                  | ((uint32_t) pci_dev->bus_number)      << PCI_CONF_ADDR_PORT_BUS_OFFS
                  | ((uint32_t) pci_dev->device_number)   << PCI_CONF_ADDR_PORT_DEV_OFFS
                  | ((uint32_t) pci_dev->function_number) << PCI_CONF_ADDR_PORT_FUN_OFFS
                  | (offset & PCI_REGISTER_OFFS_MASK);

    outl(PCI_CONFIGURATION_ADDR_PORT, addr);
    outl(PCI_CONFIGURATION_DATA_PORT, val);

    return;
}
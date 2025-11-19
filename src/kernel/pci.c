#include "pci.h"
#include "io.h"

static uint32_t pci_address_make(uint32_t bus, uint32_t device, uint32_t function, uint32_t offset)
{
    return (1U << 31) | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC);
}

static bool pci_device_exists(uint32_t id_reg)
{
    return (id_reg & 0xFFFF) != 0xFFFF;
}

static bool pci_class_match(uint32_t class_reg, uint8_t class_code, uint8_t subclass)
{
    return ((class_reg >> 24) == class_code) &&
           ((class_reg >> 16 & 0xFF) == subclass);
}

static bool pci_single_function(uint16_t bus, uint8_t device)
{
    uint32_t header_reg = pci_config_read(bus, device, 0, PCI_CONFIG_REG_HEADER);
    uint8_t header_type = (header_reg >> 16) & 0xFF;
    return (header_type & 0x80) == 0;
}

static void __pci_info_fill(struct pci_device *pdev, uint8_t bus, uint8_t device, uint8_t function, uint32_t id_reg, uint32_t class_reg)
{
    pdev->bus = bus;
    pdev->device = device;
    pdev->function = function;
    pdev->vendor_id = id_reg & 0xFFFF;
    pdev->device_id = id_reg >> 16;
    pdev->prog_if = (class_reg >> 8) & 0xFF;
}

static bool pci_match_and_fill(uint8_t bus, uint8_t device, uint8_t function, struct pci_device *pdev)
{
    uint32_t id_reg = pci_config_read(bus, device, function, PCI_CONFIG_REG_ID);
    if (!pci_device_exists(id_reg))
        return false;
    uint32_t class_reg = pci_config_read(bus, device, function, PCI_CONFIG_REG_CLASS);
    if (pci_class_match(class_reg, pdev->class_code, pdev->subclass)) {
        __pci_info_fill(pdev, bus, device, function, id_reg, class_reg);
        return true;
    }
    return false;
}

int pci_info_fill(struct pci_device *pdev)
{
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            if (pci_match_and_fill(bus, device, 0, pdev))
                return 0;
            if (!pci_single_function(bus, device)) {
                for (uint8_t function = 1; function < 8; function++) {
                    if (pci_match_and_fill(bus, device, function, pdev))
                        return 0;
                }
            }
        }
    }
    return -1;
}

uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
    outl(PCI_CONFIG_ADDRESS, pci_address_make(bus, device, function, offset));
    return inl(PCI_CONFIG_DATA);
}

void pci_config_write(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t data)
{
    outl(PCI_CONFIG_ADDRESS, pci_address_make(bus, device, function, offset));
    outl(PCI_CONFIG_DATA, data);
}
#include "pci.h"
#include "io.h"

static uint32_t pci_config_address(uint32_t bus, uint32_t device, uint32_t function, uint32_t offset)
{
    return (1U << 31) | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC);
}

static bool pci_device_exists(uint32_t id_reg)
{
    return (id_reg & 0xFFFF) != 0xFFFF;
}

static bool pci_class_matches(uint32_t class_reg, uint8_t class_code, uint8_t subclass)
{
    return ((class_reg >> 24) == class_code) &&
           ((class_reg >> 16 & 0xFF) == subclass);
}

static bool pci_is_single_function_device(uint16_t bus, uint8_t device)
{
    uint32_t header_reg = pci_config_read(bus, device, 0, 0x0C);
    uint8_t header_type = (header_reg >> 16) & 0xFF;
    return (header_type & 0x80) == 0;
}

static void pci_device_fill(struct pci_device *pdev, uint8_t bus, uint8_t device, uint8_t function, uint32_t id_reg, uint32_t class_reg)
{
    pdev->bus = bus;
    pdev->device = device;
    pdev->function = function;
    pdev->vendor_id = id_reg & 0xFFFF;
    pdev->device_id = id_reg >> 16;
    pdev->class_code = class_reg >> 24;
    pdev->subclass = (class_reg >> 16) & 0xFF;
    pdev->prog_if = (class_reg >> 8) & 0xFF;
}

uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
    outl(PCI_CONFIG_ADDRESS, pci_config_address(bus, device, function, offset));
    return inl(PCI_CONFIG_DATA);
}

bool pci_device_find(uint8_t class_code, uint8_t subclass, struct pci_device *pdev)
{
    uint32_t id_reg, class_reg;

    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            for (uint8_t function = 0; function < 8; function++) {
                id_reg = pci_config_read(bus, device, function, PCI_CONFIG_REG_ID);
                if (!pci_device_exists(id_reg))
                    break;
                class_reg = pci_config_read(bus, device, function, PCI_CONFIG_REG_CLASS);
                if (pci_class_matches(class_reg, class_code, subclass)) {
                    pci_device_fill(pdev, bus, device, function, id_reg, class_reg);
                    return true;
                }
                if (function == 0 && pci_is_single_function_device(bus, device))
                    break;
            }
        }
    }
    return false;
}
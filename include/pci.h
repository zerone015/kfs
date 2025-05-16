#ifndef _PCI_H
#define _PCI_H

#include <stdint.h>
#include <stdbool.h>

#define PCI_CONFIG_ADDRESS      0xCF8
#define PCI_CONFIG_DATA         0xCFC

// PCI Configuration Space Register Offsets (DWORD-aligned)
#define PCI_CONFIG_REG_ID             0x00  // [15:0]=Vendor ID, [31:16]=Device ID
#define PCI_CONFIG_REG_COMMAND        0x04  // [15:0]=Command, [31:16]=Status
#define PCI_CONFIG_REG_CLASS          0x08  // [7:0]=Revision ID, [15:8]=Prog IF, [23:16]=Subclass, [31:24]=Class code
#define PCI_CONFIG_REG_HEADER         0x0C  // [7:0]=Cache Line Size, [15:8]=Latency Timer, [23:16]=Header Type, [31:24]=BIST

#define PCI_CONFIG_REG_BAR0           0x10
#define PCI_CONFIG_REG_BAR1           0x14
#define PCI_CONFIG_REG_BAR2           0x18
#define PCI_CONFIG_REG_BAR3           0x1C
#define PCI_CONFIG_REG_BAR4           0x20
#define PCI_CONFIG_REG_BAR5           0x24

#define PCI_CONFIG_REG_CARDBUS_CIS    0x28
#define PCI_CONFIG_REG_SUBSYSTEM      0x2C  // [15:0]=Subsystem Vendor ID, [31:16]=Subsystem ID
#define PCI_CONFIG_REG_EXP_ROM_BASE   0x30
#define PCI_CONFIG_REG_CAP_PTR        0x34  // [7:0]=Capabilities Pointer

#define PCI_CONFIG_REG_RESERVED       0x38
#define PCI_CONFIG_REG_INTERRUPT      0x3C  // [7:0]=Interrupt Line, [15:8]=Interrupt Pin, [23:16]=Min Grant, [31:24]=Max Latency

struct pci_device {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
};

uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
bool pci_device_find(uint8_t class_code, uint8_t subclass, struct pci_device *pdev);

#endif
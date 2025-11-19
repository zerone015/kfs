#include "ata.h"
#include "io.h"
#include "pci.h"
#include "panic.h"

static struct ata_channel channels[2];
static struct ata_device devices[4];

static void ata_delay_400ns(uint8_t channel) 
{
   for(int i = 0; i < 14; i++)
      (void)inb(channels[channel].ctrl);
}

static void ata_bsy_poll(uint8_t channel)
{
   while (inb(channels[channel].base + ATA_REG_STATUS) & ATA_SR_BSY)
      ;
}

static uint32_t ata_combine_dword(uint16_t low, uint16_t high)
{
   return ((uint32_t)high << 16) | low;
}

static int ata_identity_command(uint8_t channel, uint8_t drive, uint16_t *buf)
{
   outb(channels[channel].base + ATA_REG_DEVSEL, 0xA0 | (drive << 4));
   ata_delay_400ns(channel);

   outb(channels[channel].base + ATA_REG_SECCOUNT0, 0);
   outb(channels[channel].base + ATA_REG_LBA0, 0);
   outb(channels[channel].base + ATA_REG_LBA1, 0);
   outb(channels[channel].base + ATA_REG_LBA2, 0);

   outb(channels[channel].base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
   
   if (inb(channels[channel].base + ATA_REG_STATUS) == 0)
      return -1;

   ata_bsy_poll(channel);

   if (inb(channels[channel].base + ATA_REG_LBA1) != 0 || inb(channels[channel].base + ATA_REG_LBA2) != 0)
      return -1;

   while (true) {
      uint8_t status = inb(channels[channel].base + ATA_REG_STATUS);
      if (status & ATA_SR_ERR)
         return -1;
      if (status & ATA_SR_DRQ)
         break;
   }

   for (int i = 0; i < 256; i++) {
      uint16_t data = inw(channels[channel].base);
      buf[i] = data;
   }
   
   return 0;
}

static void ata_channels_init(uint16_t bar4)
{
   channels[ATA_PRIMARY].base = ATA_PRIMARY_IO_BASE;
   channels[ATA_PRIMARY].ctrl = ATA_PRIMARY_CTRL_BASE;
   channels[ATA_PRIMARY].bmide = bar4;
   channels[ATA_SECONDARY].base = ATA_SECONDARY_IO_BASE;
   channels[ATA_SECONDARY].ctrl = ATA_SECONDARY_CTRL_BASE;
   channels[ATA_SECONDARY].bmide = bar4 + 8;
}

static void ata_controller_init(struct pci_device *ata_ctrl)
{
   uint32_t cmd_reg;

   cmd_reg = pci_config_read(ata_ctrl->bus, ata_ctrl->device, ata_ctrl->function, PCI_CONFIG_REG_COMMAND);
   cmd_reg |= PCI_CONFIG_BUS_MASTER;
   pci_config_write(ata_ctrl->bus, ata_ctrl->device, ata_ctrl->function, PCI_CONFIG_REG_COMMAND, cmd_reg);   
}

static void ata_devices_init(void)
{
   uint16_t buf[256];

   for (uint8_t i = 0; i < 4; i++) {
      uint8_t channel = i / 2;
      uint8_t drive = i % 2;

      if (ata_identity_command(channel, drive, buf) < 0)
         continue;

      devices[i].present = 1;
      devices[i].type = IDE_ATA;
      devices[i].channel = channel;
      devices[i].drive = drive;
      devices[i].signature = buf[ATA_IDENT_DEVICETYPE];
      devices[i].capabilities = buf[ATA_IDENT_CAPABILITIES];
      devices[i].command_sets = ata_combine_dword(buf[ATA_IDENT_COMMANDSETS], buf[ATA_IDENT_COMMANDSETS + 1]);

      if (devices[i].command_sets & (1 << 10))
         devices[i].size = ata_combine_dword(buf[ATA_IDENT_MAX_LBA_EXT], buf[ATA_IDENT_MAX_LBA_EXT + 1]);
      else
         devices[i].size = ata_combine_dword(buf[ATA_IDENT_MAX_LBA], buf[ATA_IDENT_MAX_LBA + 1]);

      for (int k = 0; k < 20; k++) {
         devices[i].model[k * 2] = buf[ATA_IDENT_MODEL + k] >> 8;
         devices[i].model[k * 2 + 1] = buf[ATA_IDENT_MODEL + k] & 0xFF;
      }
      devices[i].model[40] = '\0';
      for (int k = 39; k >= 0 && devices[i].model[k] == ' '; k--)
         devices[i].model[k] = '\0';
   }
}

void ata_init(void) 
{
   struct pci_device ata_ctrl;

   ata_ctrl.class_code = PCI_ATA_CTRL_CLASS_CODE;
   ata_ctrl.subclass = PCI_ATA_CTRL_SUBCLASS;

   if (pci_info_fill(&ata_ctrl) < 0)
      do_panic("ATA controller not found on PCI bus");

   ata_controller_init(&ata_ctrl);

   uint32_t bar4_reg = pci_config_read(ata_ctrl.bus, ata_ctrl.device, ata_ctrl.function, PCI_CONFIG_REG_BAR4);
   if (bar4_reg == 0)
      do_panic("Invalid BAR4 for ATA controller");
   ata_channels_init((uint16_t)bar4_reg);
   
   ata_devices_init();
}
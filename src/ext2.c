#include "ext2.h"
#include "ata.h"
#include "hmm.h"

struct ext2_super_block *sblock;
struct ext2_group_desc *bgroups;
struct ext2_inode *inodes;
uint32_t **block_maps;
uint32_t **inode_maps;

void filesystem_init(void)
{
    sblock = (struct ext2_super_block *)kmalloc(1024);
    if (!sblock)
        do_panic("filesystem initialize failed");
    
    ata_pio_read(EXT2_SUPER_BLOCK_LBA, 2, (uint16_t *)sblock);

    if (sblock->s_magic != EXT2_MAGIC)
        do_panic("Not an EXT2 filesystem");
    
    uint32_t block_size = 1024 << sblock->s_log_block_size;
    uint32_t block_groups = (sblock->s_blocks_count + sblock->s_blocks_per_group - 1) / sblock->s_blocks_per_group;

    bgroups = (struct ext2_group_desc *)kmalloc(block_groups * sizeof(struct ext2_group_desc));
    if (!bgroups)
        do_panic("failed to allocate block group descriptor table");
    //..
}
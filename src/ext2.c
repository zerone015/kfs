#include "ext2.h"
#include "ata.h"
#include "hmm.h"

struct ext2_super_block *sblock;
struct ext2_group_desc *bgroups;

void ext2_init(void)
{
    sblock = (struct ext2_super_block *)kmalloc(sizeof(struct ext2_super_block));
    if (!sblock)
        do_panic("filesystem initialization failed");
    
    ata_pio_read(2, 2, (uint16_t *)sblock);

    if (sblock->s_magic != EXT2_MAGIC)
        do_panic("Not an EXT2 filesystem");
    
    size_t block_size = 1024 << sblock->s_log_block_size;
    size_t block_group_nr = (sblock->s_blocks_count + sblock->s_blocks_per_group - 1) / sblock->s_blocks_per_group;

    size_t block_group_size = block_group_nr * sizeof(struct ext2_group_desc);
    bgroups = (struct ext2_group_desc *)kmalloc(block_group_size);
    if (!bgroups)
        do_panic("failed to allocate block group descriptor table");
    
    size_t buf_size = align_up(block_group_size, 512);
    void *buf = kmalloc(buf_size);
    if (!buf)
        do_panic("failed to allocate block group descriptor table");
    
    ata_pio_read(4, buf_size / 512, buf);

    memcpy(bgroups, buf, block_group_size);
    kfree(buf);

    // nIEN 비트 지워야함
}
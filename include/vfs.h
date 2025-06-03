#ifndef _VFS_H
#define _VFS_H

#include <stdint.h>
#include "ext2.h"
#include "list.h"

#define DENTRY_HASH_BUCKETS 1024
#define NAME_MAX            255

struct inode {
    uint32_t nr;
    uint16_t mode;
    uint32_t flags;
    uint16_t links_count;
    uint32_t ref_count;
    uint16_t uid;
    uint16_t gid;
    uint64_t size;
    uint32_t atime;
    uint32_t ctime;
    uint32_t mtime;
    uint32_t block[15];
};

struct dentry {
    char *name;
    struct inode *inode;
    struct dentry *parent;
    struct hlist_node hnode;
    struct list_head children;
    struct list_head child; 
    uint32_t ref_count;
};

struct file {
    struct dentry *dentry;
    long long pos;
    int flags;
    int mode;
};

#endif
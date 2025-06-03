#include "vfs.h"
#include "hash.h"
#include "utils.h"

struct hlist_head dentry_table[DENTRY_HASH_BUCKETS];

static int dentry_hash(const char *name, uint32_t parent_inode) 
{
    char buf[NAME_MAX + sizeof(uint32_t) + 1];
    size_t len = 0;
    
    while (name[len])
        buf[len] = name[len++];

    memcpy(buf + len, &parent_inode, sizeof(uint32_t));
    buf[len + sizeof(uint32_t)] = '\0';

    return jenkins_hash(buf) % DENTRY_HASH_BUCKETS;
}

void dentry_register(struct dentry *dentry)
{
    int hash = dentry_hash(dentry->name, dentry->parent->inode->nr);
    hlist_add_head(&dentry->hnode, &dentry_table[hash]);
}

struct dentry *dentry_lookup(const char *name, uint32_t parent_inode)
{
    int hash = dentry_hash(name, parent_inode);
    struct hlist_head *hlist_head = &dentry_table[hash];
    struct dentry *cur;

    hlist_for_each_entry(cur, hlist_head, hnode) {
        // name cmp and  parent_inode cmp..?
        // 매번 name cmp 하는거 좀 그렇네 
    }
    return cur;
}
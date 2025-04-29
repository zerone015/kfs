#include "proc.h"
#include "pid.h"

struct list_head process_list;
struct hlist_head process_table[PID_HASH_BUCKETS];
struct hlist_head pgroup_table[PGROUP_HASH_BUCKETS];

void proc_init(void)
{
    pidmap_init();
    init_list_head(&process_list);
}
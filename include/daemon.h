#ifndef _DAEMON_H
#define _DAEMON_H

#define INIT_PROCESS_PID        1
#define KERNEL_STACK_SIZE       (PAGE_SIZE * 2)

void init_process(void);

#endif
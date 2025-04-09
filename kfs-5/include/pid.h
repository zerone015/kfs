#ifndef _PID_H
#define _PID_H

#define PID_MAX     32768
#define PIDMAP_MAX  (PID_MAX / 8 / 4)
#define PID_NONE    -1

int alloc_pid(void);
void free_pid(int pid);

#endif
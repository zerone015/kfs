#ifndef _UNISTD_H
#define _UNISTD_H

int fork(void);
void exit(int status);
int wait(int *status);
void write(const char *msg);
int getuid(void);

#endif
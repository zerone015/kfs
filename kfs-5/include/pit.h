#ifndef _PIT_H
#define _PIT_H

#define PIT_CHANNEL0_DATA 0x40
#define PIT_COMMAND       0x43
#define PIT_MODE          0x36  // 채널 0, lobyte/hibyte, rate generator 모드
#define PIT_FREQUENCY     1193182

void pit_init(void);

#endif
#ifndef __TIMER_H__

#define __TIMER_H__

unsigned long volatile jiffies = 0;

void timer_init();

void do_timer();

#endif

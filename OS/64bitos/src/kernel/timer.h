#ifndef __TIMER_H__

#define __TIMER_H__

#include "lib.h"

unsigned long volatile jiffies = 0;

void timer_init();

void do_timer();

struct timer_list
{
    struct List list;
    unsigned long expire_jiffies; // 执行期望时间
    void (*func)(void *data);     // 处理方法
    void *data;                   // 参数 void 指针
};

struct timer_list timer_list_head;

#endif

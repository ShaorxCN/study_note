#ifndef __TIMER_H__

#define __TIMER_H__

#include "lib.h"

unsigned long volatile jiffies = 0;

void timer_init();

void do_timer();

struct timer_list
{
    struct List list;
    unsigned long expire_jiffies; // 执行期望时间 或者 过期时间 这边单位是s 超过多少s就不执行了 软中断进来 这边1s一次累加 感觉有可能会超过？
    void (*func)(void *data);     // 处理方法
    void *data;                   // 参数 void 指针
};

struct timer_list timer_list_head;

#endif

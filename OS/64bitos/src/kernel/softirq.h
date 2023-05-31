#ifndef __SOFTIRQ_H__
#define __SOFTIRQ_H__
// 置位softirq_status 中定时器bit位的宏
#define TIMER_SIRQ (1 << 0)

// 纪录各个软中断的状态 这边暂定64个软中断 按位置位
unsigned long softirq_status = 0;

struct softirq
{
    void (*action)(void *data);
    void *data;
};

struct softirq softirq_vector[64] = {0};

void register_softirq(int nr, void (*action)(void *data), void *data);
void unregister_softirq(int nr);

void set_softirq_status(unsigned long status);
unsigned long get_softirq_status();

void softirq_init();

#endif
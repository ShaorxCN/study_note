#include "softirq.h"
#include "lib.h"

void set_softirq_status(unsigned long status)
{
    softirq_status |= status;
}

unsigned long get_softirq_status()
{
    return softirq_status;
}

void register_softirq(int nr, void (*action)(void *data), void *data)
{
    softirq_vector[nr].action = action;
    softirq_vector[nr].data = data;
}
void unregister_softirq(int nr)
{
    softirq_vector[nr].action = NULL;
    softirq_vector[nr].data = NULL;
}

void softirq_init()
{
    softirq_status = 0;
    memset(softirq_vector, 0, sizeof(struct softirq) * 64);
}

// 这边处理软中断过程中允许中断
void do_softirq()
{
    int i;
    sti(); // 这里提前允许中断 这样实现了将定时器等软中断放到中断下半部分的目的

    for (i = 0; i < 64 && softirq_status; i++)
    {
        if (softirq_status & (1 << i))
        {
            softirq_vector[i].action(softirq_vector[i].data);
            softirq_status &= ~(1 << i);
        }
    }

    cli();
}
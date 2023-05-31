#include "timer.h"
#include "softirq.h"
#include "printk.h"
#include "lib.h"

void timer_init()
{
    jiffies = 0;
    register_softirq(0, &do_timer, NULL);
}

void do_timer(void *data)
{
    color_printk(RED, WHITE, "(HPET:%ld)", jiffies);
}

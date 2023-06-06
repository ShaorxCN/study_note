#ifndef __PREEMPT_H__
#define __PREEMPT_H__

#include "task.h"

// 使可抢占(可能增加)
#define preempt_enable()          \
    do                            \
    {                             \
        current->preempt_count--; \
    } while (0)

#define preempt_disable()         \
    do                            \
    {                             \
        current->preempt_count++; \
    } while (0)
#endif
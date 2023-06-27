#ifndef __PREEMPT_H__
#define __PREEMPT_H__

#include "task.h"


// 自旋锁 如果持有自旋锁则关闭抢占功能 防止多个进程访问同一共享资源
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
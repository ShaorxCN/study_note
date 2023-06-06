#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#include "preempt.h"

// 自旋锁类型
typedef struct
{
    __volatile__ unsigned long lock; // 1 unlock 0 lock
} spinlock_T;

static inline void spin_init(spinlock_T *lock)
{
    lock->lock = 1;
}

// jns sf=0 则跳转 sf sf=1 表示计算结果负数 也就说已经是锁得状态 sf= 0 说明上锁 则结束 否则就是说明已经锁了 到2
// pause 空转指令 周期数不一定 主要是降低功耗以及可能得效率 2中比较0和lock得值 如果没有解锁则继续2 否则到1开始上锁
static inline void spin_lock(spinlock_T *lock)
{
    preempt_disable();
    __asm__ __volatile__("1:\n\t"
                         "lock decq %0 \n\t"
                         "jns 3f \n\t"
                         "2: \n\t"
                         "pause \n\t"
                         "cmpq $0,%0 \n\t"
                         "jle 2b \n\t"
                         "jmp 1b \n\t"
                         "3: \n\t"
                         : "=m"(lock->lock)
                         :
                         : "memory");
}

static inline void spin_unlock(spinlock_T *lock)
{
    __asm__ __volatile__("movq $1,%0 \n\t"
                         : "=m"(lock->lock)
                         :
                         : "memory");
    preempt_enable();
}

// 尝试锁 这边尝试交换数值 然后检查 如果不是0那么就是没锁上 那就还是抢占enable
static inline long spin_trylock(spinlock_T *lock)
{
    unsigned long tmp_value = 0;
    preempt_disable();
    __asm__ __volatile__("xchgq	%0,	%1	\n\t"
                         : "=q"(tmp_value), "=m"(lock->lock)
                         : "0"(0)
                         : "memory");
    if (!tmp_value)
        preempt_enable();
    return tmp_value;
}

#endif
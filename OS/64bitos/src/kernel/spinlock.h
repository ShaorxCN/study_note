#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__
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
}

#endif
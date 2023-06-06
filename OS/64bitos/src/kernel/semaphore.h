#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__
#include "atomic.h"
#include "lib.h"
#include "task.h"
#include "schedule.h"

typedef struct
{
    struct List wait_list;
    struct task_struct *tsk;
} wait_queue_T;

void wait_queue_init(wait_queue_T *wait_queue, struct task_struct *tsk)
{
    list_init(&wait_queue->wait_list);
    wait_queue->tsk = tsk;
}

typedef struct
{
    atomic_T counter;  // 统计可用资源的数量
    wait_queue_T wait; // 等待队列
} semaphore_T;

void semaphore_init(semaphore_T *semaphore, unsigned long count)
{
    atomic_set(&semaphore->counter, count);
    wait_queue_init(&semaphore->wait, NULL);
}

// 获取失败  加入等待队列 并且等待调度
void __down(semaphore_T *semaphore)
{
    wait_queue_T wait;
    wait_queue_init(&wait, currnet);

    current->state = TASK_UNINTERRUPTIBLE;
    list_add_to_before(&semaphore->wait.wait_list, &wait.wait_list);
    schedule();
}

// 获取信号量
void semaphore_down(semaphore_T *semaphore)
{
    // 有可用资源 那就是直接获取并且减减
    if (atomic_read(semaphore->counter) > 0)
        atomic_dec(&semaphore->counter);
    else
        __down(semaphore);
}

// 将等待队列中的一个任务插入cpu任务队列
void __up(semaphore_T *semaphore)
{
    wait_queue_T *wait = container_of(list_next(&semaphore->wait.wait_list), wait_queue_T, wait_list);

    list_del(&wait->wait_list);
    wait->state = TASK_RUNNING;
    insert_task_queue(wait->tsk);
}

// 释放信号量
void semaphore_up(semaphore_T *semaphore)
{
    // 有可用资源 那就是直接获取并且减减
    if (list_is_empty(&semaphore->wait.wait_list))
        atomic_inc(&semaphore->counter);
    else
        __up(semaphore);
}
#endif
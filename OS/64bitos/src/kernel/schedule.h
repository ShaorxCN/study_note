#ifndef __SCHEDULE_H__

#define __SCHEDULE_H__

#include "task.h"

struct schedule
{
    long running_task_count;       // 当前队列的进程数量
    long CPU_exec_task_jiffies;    // 调度时保存进程可执行的时间片数量
    struct task_struct task_queue; // 队列头
};

extern struct schedule task_schedule;

void schedule();
void schedule_init();
struct task_struct *get_next_task();
void insert_task_queue(struct task_struct *tsk);

#endif
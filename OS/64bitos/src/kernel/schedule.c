#include "task.h"
#include "lib.h"
#include "printk.h"
#include "timer.h"
#include "schedule.h"

struct schedule task_schedule;

void schedule_init()
{
    memset(&task_schedule, 0, sizeof(struct schedule));
    list_init(&task_schedule.task_queue.list);
    task_schedule.running_task_count = 1; // 放置的idle 进程  当无任务的时候执行该idle
    task_schedule.task_queue.vrun_time = 0x7fffffffffffffff;
    task_schedule.CPU_exec_task_jiffies = 4;
}

struct task_struct *get_next_task()
{
    struct task_struct *tsk = NULL;
    if (list_is_empty(&task_schedule.task_queue.list))
    {
        return &init_task_union.task; // idle
    }

    tsk = container_of(list_next(&task_schedule.task_queue.list), struct task_struct, list);
    list_del(&tsk->list);
    task_schedule.running_task_count -= 1;
    return tsk;
}

void insert_task_queue(struct task_struct *tsk)
{
    struct task_struct *tmp = container_of(list_next(&task_schedule.task_queue.list), struct task_struct, list);
    if (tsk == &init_task_union.task)
        return;

    if (list_is_empty(&task_schedule.task_queue.list))
    {
    }
    else
    {
        while (tmp->vrun_time < tsk->vrun_time)
            tmp = container_of(list_next(&tmp->list), struct task_struct, list);
    }
    list_add_to_before(&tmp->list, &tsk->list);
    task_schedule.running_task_count += 1;
}

void schedule()
{
    struct task_struct *tsk = NULL;
    current->flags &= ~NEED_SCHEDULE;
    tsk = get_next_task();
    color_printk(RED, BLACK, "schedule:%d#\n", jiffies);

    // 检查vrun_time 如果小于继续执行 大于则切换
    if (current->vrun_time >= tsk->vrun_time)
    {
        // running则继续等待调度执行
        if (current->state == TASK_RUNNING)
            insert_task_queue(current);

        // 说明不是时间片耗尽引发的 重新保存进程可执行的时间片数量？
        if (!task_schedule.CPU_exec_task_jiffies)
            switch (tsk->priority)
            {
            case 0:
            case 1:
                task_schedule.CPU_exec_task_jiffies = 4 / task_schedule.running_task_count;
                break;
            case 2:
            default:
                task_schedule.CPU_exec_task_jiffies = 4 / task_schedule.running_task_count * 3;
                break;
            }
        switch_to(current, tsk);
    }
    else
    {
        insert_task_queue(tsk);

        if (!task_schedule.CPU_exec_task_jiffies)
            switch (tsk->priority)
            {
            case 0:
            case 1:
                task_schedule.CPU_exec_task_jiffies = 4 / task_schedule.running_task_count;
                break;
            case 2:
            default:
                task_schedule.CPU_exec_task_jiffies = 4 / task_schedule.running_task_count * 3;
                break;
            }
    }
}
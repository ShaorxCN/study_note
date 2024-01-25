#include "task.h"
#include "lib.h"
#include "printk.h"
#include "timer.h"
#include "schedule.h"
#include "SMP.h"

struct schedule task_schedule[NR_CPUS];

void schedule_init()
{
    int i = 0;

    memset(&task_schedule, 0, sizeof(struct schedule) * NR_CPUS);

    for (i = 0; i < NR_CPUS; i++)
    {
        list_init(&task_schedule[i].task_queue.list);
        task_schedule[i].running_task_count = 1; // 放置的idle 进程  当无任务的时候执行该idle
        task_schedule[i].task_queue.vrun_time = 0x7fffffffffffffff;
        task_schedule[i].CPU_exec_task_jiffies = 4;
    }
}

struct task_struct *get_next_task()
{
    struct task_struct *tsk = NULL;
    if (list_is_empty(&task_schedule[SMP_cpu_id()].task_queue.list))
    {
        return init_task[SMP_cpu_id()]; // idle
    }

    tsk = container_of(list_next(&task_schedule[SMP_cpu_id()].task_queue.list), struct task_struct, list);
    list_del(&tsk->list);
    task_schedule[SMP_cpu_id()].running_task_count -= 1;
    return tsk;
}

void insert_task_queue(struct task_struct *tsk)
{
    struct task_struct *tmp = NULL;

    if (tsk == init_task[SMP_cpu_id()])
        return;
    tmp = container_of(list_next(&task_schedule[SMP_cpu_id()].task_queue.list), struct task_struct, list);

    if (list_is_empty(&task_schedule[SMP_cpu_id()].task_queue.list))
    {
    }
    else
    {
        while (tmp->vrun_time < tsk->vrun_time)
            tmp = container_of(list_next(&tmp->list), struct task_struct, list);
    }
    list_add_to_before(&tmp->list, &tsk->list);
    task_schedule[SMP_cpu_id()].running_task_count += 1;
}

void schedule()
{
    struct task_struct *tsk = NULL;
    long cpu_id = SMP_cpu_id();
    current->flags &= ~NEED_SCHEDULE;
    tsk = get_next_task();
    color_printk(RED, BLACK, "schedule:%d#\n", jiffies);
    color_printk(RED, BLACK, "current :%p, tsk:%p#\n", current, tsk);

    // 检查vrun_time  说明执行的时间已经长了 切换
    if (current->vrun_time >= tsk->vrun_time)
    {
        // running则继续等待调度执行
        if (current->state == TASK_RUNNING)
            insert_task_queue(current);

        // 说明是时间片耗尽引发的 重新根据进程优先级设置保存进程可执行的时间片
        if (!task_schedule[cpu_id].CPU_exec_task_jiffies)
            switch (tsk->priority)
            {
            case 0:
            case 1:
                task_schedule[cpu_id].CPU_exec_task_jiffies = 4 / task_schedule[cpu_id].running_task_count;
                break;
            case 2:
            default:
                task_schedule[cpu_id].CPU_exec_task_jiffies = 4 / task_schedule[cpu_id].running_task_count * 3;
                break;
            }
        // 这边只有中断才会有调度节点 所以原始进程的线程已经保存
        switch_to(current, tsk);
    }
    else
    {
        // 这里表示不应该切换 所以重新插入tsk
        insert_task_queue(tsk);

        // 检查是否是cpu时间片耗尽引发的中断
        if (!task_schedule[SMP_cpu_id()].CPU_exec_task_jiffies)
            switch (tsk->priority)
            {
            case 0:
            case 1:
                task_schedule[SMP_cpu_id()].CPU_exec_task_jiffies = 4 / task_schedule[SMP_cpu_id()].running_task_count;
                break;
            case 2:
            default:
                task_schedule[SMP_cpu_id()].CPU_exec_task_jiffies = 4 / task_schedule[SMP_cpu_id()].running_task_count * 3;
                break;
            }
    }
}
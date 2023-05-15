#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__
#include "linkage.h"
#include "ptrace.h"

// 中断的一些操作接口
typedef struct hw_int_type
{
    // 使能中断操作接口
    void (*enable)(unsigned long irq);
    // 禁止中断操作接口
    void (*disable)(unsigned long irq);
    // 安装中断操作接口
    unsigned long (*install)(unsigned long irq, void *arg);
    // 卸载中断操作接口
    void (*uninstall)(unsigned long irq);
    // 应答中断操作接口
    void (*ack)(unsigned long irq);
} hw_int_controller;

typedef struct
{
    hw_int_controller *controller;
    // 中断名
    char *irq_name;
    // 中断处理程序需要的参数
    unsigned long parameter;
    // 中断处理程序函数指针
    void (*handler)(unsigned long nr, unsigned long parameter, struct pt_regs *regs);
    // 自定义标志位
    unsigned long flags;
} irq_desc_T;

#define NR_IRQS 24

irq_desc_T interrupt_desc[NR_IRQS] = {0};
irq_desc_T SMP_IPI_desc[10] = {0};

int register_irq(unsigned long irq,
                 void *arg,
                 void (*handler)(unsigned long nr, unsigned long parameter, struct pt_regs *regs),
                 unsigned long parameter,
                 hw_int_controller *controller,
                 char *irq_name);

int unregister_irq(unsigned long irq);

extern void (*interrupt[24])(void);

extern void do_IRQ(struct pt_regs *regs, unsigned long nr);
extern void (*SMP_interrupt[10])(void);
#endif
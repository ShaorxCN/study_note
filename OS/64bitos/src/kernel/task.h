#ifndef __TASK_H__
#define __TASK_H__

#include "lib.h"
#include "memory.h"
#include "cpu.h"
#include "ptrace.h"

// 模式进入kernel后手动跟新了cs
#define KERNEL_CS (0x08)
#define KERNEL_DS (0x10)

#define USER_CS (0x28)
#define USER_DS (0x30)

#define CLONE_FS (1 << 0)
#define CLONE_FILES (1 << 1)
#define CLONE_SIGNAL (1 << 2)

// stack size 32K
#define STACK_SIZE 32768

extern char _text;
extern char _etext;
extern char _data;
extern char _edata;
extern char _rodata;
extern char _erodata;
extern char _bss;
extern char _ebss;
extern char _end;

// 就是第一个进程（ldle）的内核栈顶
extern unsigned long _stack_start;

// 进程状态

#define TASK_RUNNING (1 << 0)
#define TASK_INTERRUPTIBLE (1 << 1)
#define TASK_UNINTERRUPTIBLE (1 << 2)
#define TASK_ZOMBIE (1 << 3)
#define TASK_STOPPED (1 << 4)

// 内存描述结构 用户空间的
struct mm_struct
{
    pml4t_t *pgd; // 页表指针 cr3
    unsigned long start_code, end_code;
    unsigned long start_data, end_data;
    unsigned long start_rodata, end_rodata;
    unsigned long start_brk, end_brk; // 动态内存分配区
    unsigned long start_stack;        // 应用层栈基地址
};

// 进程切换时保存现场用结构
struct thread_struct
{
    unsigned long rsp0; // in tss 在内核层中使用的栈指针 内核栈指针
    unsigned long rip;  // 切换回来的代码指针
    unsigned long rsp;  // 进程切换时的栈指针
    unsigned long fs;
    unsigned long gs;

    unsigned long cr2;
    unsigned long trap_nr; // 异常产生时候的异常号
    unsigned long error_code;
};

// 内核线程(核心进程)
#define PF_KTHREAD (1 << 0)

// pcb
struct task_struct
{
    struct List list;    // 双向链表 连接各进程pcb
    volatile long state; // 记录任务状态 运行态 停止态 可中断态  volatile 说明不要优化 每次使用重新读取 不要使用寄存器中的备份值
    unsigned long flags; // 任务标志:进程 线程 内核线程

    struct mm_struct *mm;         // 内存空间结构体 包含页表和程序段信息
    struct thread_struct *thread; // 进程切换时保留的状态信息 比如rsp0 ip等
    /*
    0x0000,0000,0000,0000 - 0x0000,7fff,ffff,ffff user
    0xffff,8000,0000,0000 - 0xffff,ffff,ffff,ffff kernel 高位作为内核
    e */
    unsigned long addr_limit; // 进程空间地址范围

    long pid;      // 进程pid
    long counter;  // 进程可用时间片
    long signal;   // 进程持有的信号
    long priority; // 进程优先级
};

// 内核为每个进程创建的管理  和内核栈一体 低位放task_struct 高地址则作为内核层栈空间使用
union task_union
{
    struct task_struct task;
    unsigned long stack[STACK_SIZE / sizeof(unsigned long)]; // 该进程使用的内核栈  也就是linux进程内核栈 暂定32k 不够再扩展
} __attribute__((aligned(8)));                               // 实际整体是按照32k对齐 见lds文件 也就是结构体放在低地址 然后上方是栈空间使用

struct mm_struct init_mm;

// 定义第一个进程
struct thread_struct init_thread;

// 定义第一个进程初始化 实际应该是ldle?
#define INIT_TASK(tsk)                    \
    {                                     \
        .state = TASK_UNINTERRUPTIBLE,    \
        .flags = PF_KTHREAD,              \
        .mm = &init_mm,                   \
        .thread = &init_thread,           \
        .addr_limit = 0xffff800000000000, \
        .pid = 0,                         \
        .counter = 1,                     \
        .signal = 0,                      \
        .priority = 0                     \
    }

// 声明并初始化init_task_union
// 表示将init_task_union 放到 .data.init_task的section 里  将task_struct 初始化 手动创建第一个进程的结构
union task_union init_task_union __attribute__((__section__(".data.init_task"))) = {INIT_TASK(init_task_union.task)};

//  这是为多核准备的 每个核心对应一个初始化进程控制结构体。现在只有index0 实例化了
struct task_struct *init_task[NR_CPUS] = {&init_task_union.task, 0};

// 内存纪录结构
struct mm_struct init_mm = {0};

struct thread_struct init_thread =
    {
        .rsp0 = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),
        .rsp = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),
        .fs = KERNEL_DS,
        .gs = KERNEL_DS,
        .cr2 = 0,
        .trap_nr = 0,
        .error_code = 0};

// tss 结构 保存切换shi
// rsp0-2分别是0环 1环 2环 的默认栈指针 3环是用户层 每次3环进0环的rsp3是随机  IST是特殊栈指针 给中断有关
struct tss_struct
{
    unsigned int reserved0;
    unsigned long rsp0;
    unsigned long rsp1;
    unsigned long rsp2;
    unsigned long reserved1;
    unsigned long ist1;
    unsigned long ist2;
    unsigned long ist3;
    unsigned long ist4;
    unsigned long ist5;
    unsigned long ist6;
    unsigned long ist7;
    unsigned long reserved2;
    unsigned short reserved3;
    unsigned short iomapbaseaddr;
} __attribute__((packed));

// 定义第一个进程的tss
#define INIT_TSS                                                                             \
    {                                                                                        \
        .reserved0 = 0,                                                                      \
        .rsp0 = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)), \
        .rsp1 = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)), \
        .rsp2 = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)), \
        .reserved1 = 0,                                                                      \
        .ist1 = 0xffff800000007c00,                                                          \
        .ist2 = 0xffff800000007c00,                                                          \
        .ist3 = 0xffff800000007c00,                                                          \
        .ist4 = 0xffff800000007c00,                                                          \
        .ist5 = 0xffff800000007c00,                                                          \
        .ist6 = 0xffff800000007c00,                                                          \
        .ist7 = 0xffff800000007c00,                                                          \
        .reserved2 = 0,                                                                      \
        .reserved3 = 0,                                                                      \
        .iomapbaseaddr = 0                                                                   \
    }

// 0 ... m 表示0到m 双闭区间 一定要有空格间隔 每个cpu一个
struct tss_struct init_tss[NR_CPUS] = {[0 ... NR_CPUS - 1] = INIT_TSS};

// 获取当前ts  32k-1 取反 然后and就是向下对齐 因为是栈指针rsp 那就是其实相当于取低位的ts
static inline struct task_struct *get_current()
{
    struct task_struct *current = NULL;
    __asm__ __volatile__("andq %%rsp,%0	\n\t"
                         : "=r"(current)
                         : "0"(~32767UL));
    return current;
}

#define current get_current()

#define GET_CURRENT        \
    "movq	%rsp,	%rbx	\n\t" \
    "andq	$-32768,%rbx	\n\t"

// 进程切换函数  首先保存prev的rsp和rip 然后将next的rsp值存放到rsp寄存器中 此时将next 的rip压栈 这时候通过ret指令弹栈进入next模块
// 参数传递 rdi rsi
#define switch_to(prev, next)                                                                       \
    do                                                                                              \
    {                                                                                               \
        __asm__ __volatile__("pushq	%%rbp	\n\t"                                                     \
                             "pushq	%%rax	\n\t"                                                     \
                             "movq	%%rsp,	%0	\n\t"                                                  \
                             "movq	%2,	%%rsp	\n\t"                                                  \
                             "leaq	1f(%%rip),	%%rax	\n\t"                                           \
                             "movq	%%rax,	%1	\n\t"                                                  \
                             "pushq	%3		\n\t"                                                       \
                             "jmp	__switch_to	\n\t"                                                 \
                             "1:	\n\t"                                                              \
                             "popq	%%rax	\n\t"                                                      \
                             "popq	%%rbp	\n\t"                                                      \
                             : "=m"(prev->thread->rsp), "=m"(prev->thread->rip)                     \
                             : "m"(next->thread->rsp), "m"(next->thread->rip), "D"(prev), "S"(next) \
                             : "memory");                                                           \
    } while (0)

unsigned long do_fork(struct pt_regs *regs, unsigned long clone_flags, unsigned long stack_start, unsigned long stack_size);
void task_init();
#endif
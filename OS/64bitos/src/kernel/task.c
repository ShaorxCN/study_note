#include "task.h"
#include "ptrace.h"
#include "lib.h"
#include "memory.h"
#include "printk.h"
#include "linkage.h"
#include "gate.h"

// init进程创建函数
unsigned long init(unsigned long arg)
{
    color_printk(RED, BLACK, "init task is running,arg:%#018lx\n", arg);

    return 1;
}

unsigned long do_fork(struct pt_regs *regs, unsigned long clone_flags, unsigned long stack_start, unsigned long stack_size)
{
    struct task_struct *tsk = NULL;
    struct thread_struct *thd = NULL;
    struct Page *p = NULL;

    // 分配物理内存页
    color_printk(WHITE, BLACK, "alloc_pages,bitmap:%#018lx\n", *memory_management_struct.bits_map);

    p = alloc_pages(ZONE_NORMAL, 1, PG_PTable_Maped | PG_Active | PG_Kernel);

    color_printk(WHITE, BLACK, "alloc_pages,bitmap:%#018lx\n", *memory_management_struct.bits_map);

    tsk = (struct task_struct *)Phy_To_Virt(p->PHY_address);
    color_printk(WHITE, BLACK, "struct task_struct address:%#018lx\n", (unsigned long)tsk);

    memset(tsk, 0, sizeof(*tsk));
    *tsk = *current;

    list_init(&tsk->list);
    list_add_to_before(&init_task_union.task.list, &tsk->list);
    tsk->pid++;
    tsk->state = TASK_UNINTERRUPTIBLE;
    // 新开页中往下 开辟给thd
    thd = (struct thread_struct *)(tsk + 1);
    tsk->thread = thd;

    // 手动入栈pt_regs 模拟现场
    memcpy(regs, (void *)((unsigned long)tsk + STACK_SIZE - sizeof(struct pt_regs)), sizeof(struct pt_regs));

    // 设置thread_struct 结构
    thd->rsp0 = (unsigned long)tsk + STACK_SIZE;
    thd->rip = regs->rip;
    thd->rsp = (unsigned long)tsk + STACK_SIZE - sizeof(struct pt_regs);

    // 如果不是内核进程 那么需要改为从ret_from_intr 作为入口 进入用户层
    if (!(tsk->flags & PF_KTHREAD))
        thd->rip = regs->rip = (unsigned long)ret_from_intr;

    tsk->state = TASK_RUNNING;

    return 0;
}

unsigned long do_exit(unsigned long code)
{
    color_printk(RED, BLACK, "exit task is running,arg:%#018lx\n", code);
    while (1)
        ;
}

// 内联汇编 直接写指令即可
// 引导程序（布置执行现场环境）
// 因为之前rsp模拟到栈顶偏移pt_regs 处 所以pop出来 然后call到rbx保存的执行入口
// 0x38 上移到原本rdx的位置
// 这里一直报错 连接出来symbol对应的地址是对的 但是执行的时候 这个kernel_thread_func的线性地址会跑到一个很奇怪的地方 先改了
// 后续发现rbx的值报错 这里因为换成函数的形式 所以多了一些栈的准备工作 导致pop指令错位 所以报错
// 调用init报错
extern void kernel_thread_func(void);
__asm__(
    "kernel_thread_func:	\n\t"
    // void kernel_thread_func(void)
    // {
    //     __asm__ __volatile__(
    "	popq	%r15	\n\t"
    "	popq	%r14	\n\t"
    "	popq	%r13	\n\t"
    "	popq	%r12	\n\t"
    "	popq	%r11	\n\t"
    "	popq	%r10	\n\t"
    "	popq	%r9	\n\t"
    "	popq	%r8	\n\t"
    "	popq	%rbx	\n\t"
    "	popq	%rcx	\n\t"
    "	popq	%rdx	\n\t"
    "	popq	%rsi	\n\t"
    "	popq	%rdi	\n\t"
    "	popq	%rbp	\n\t"
    "	popq	%rax	\n\t"
    "	movq	%rax,	%ds	\n\t"
    "	popq	%rax		\n\t"
    "	movq	%rax,	%es	\n\t"
    "	popq	%rax		\n\t"
    "	addq	$0x38,	%rsp	\n\t" /////////////////////////////////
    "	movq	%rdx,	%rdi	\n\t"
    "	callq	*%rbx		\n\t" // rax存放的返回值 继续给rdi作为参数 传递给do_exit
    "	movq	%rax,	%rdi	\n\t"
    "	callq	do_exit		\n\t");

unsigned long get_kernel_fn()
{
    unsigned long __address;
    __asm__ __volatile__("leaq 	kernel_thread_func(%%rip),%0\n\t"
                         : "=r"(__address));
    return __address;
}

// 这里实际是创建内核线程 这里都没有分配用户空间
int kernel_thread(unsigned long (*fn)(unsigned long), unsigned long arg, unsigned long flags)
{
    struct pt_regs regs;
    memset(&regs, 0, sizeof(regs));

    // 程序入口和参数
    regs.rbx = (unsigned long)fn;
    regs.rdx = (unsigned long)arg;

    regs.ds = KERNEL_DS;
    regs.es = KERNEL_DS;
    regs.cs = KERNEL_CS;
    regs.ss = KERNEL_DS;
    regs.rflags = (1 << 9);

    regs.rip = get_kernel_fn();

    return do_fork(&regs, flags, 0, 0);
}

// retq 返回到 next-->rip
void __switch_to(struct task_struct *prev, struct task_struct *next)
{
    // 保存0环栈指针
    init_tss[0].rsp0 = next->thread->rsp0;

    set_tss64(init_tss[0].rsp0, init_tss[0].rsp1, init_tss[0].rsp2, init_tss[0].ist1, init_tss[0].ist2, init_tss[0].ist3, init_tss[0].ist4, init_tss[0].ist5, init_tss[0].ist6, init_tss[0].ist7);
    __asm__ __volatile__("movq	%%fs,	%0 \n\t"
                         : "=a"(prev->thread->fs));
    __asm__ __volatile__("movq	%%gs,	%0 \n\t"
                         : "=a"(prev->thread->gs));

    __asm__ __volatile__("movq	%0,	%%fs \n\t" ::"a"(next->thread->fs));
    __asm__ __volatile__("movq	%0,	%%gs \n\t" ::"a"(next->thread->gs));

    color_printk(WHITE, BLACK, "prev->thread->rsp0:%#018lx\n", prev->thread->rsp0);
    color_printk(WHITE, BLACK, "next->thread->rsp0:%#018lx\n", next->thread->rsp0);
}

// 完善第一个进程并且切换到
void task_init()
{
    struct task_struct *p = NULL;
    init_mm.pgd = (pml4t_t *)Global_CR3;

    // 这里 ldle进程是一直再内核空间的 所以mm_struct 纪录的不是应用程序信息 而是内核程序的各个段信息以及内核层栈基地址
    // _stack_start 从head.S中标记
    init_mm.start_code = memory_management_struct.start_code;
    init_mm.end_code = memory_management_struct.end_code;

    init_mm.start_data = (unsigned long)&_data;
    init_mm.end_data = memory_management_struct.end_data;

    init_mm.start_rodata = (unsigned long)&_rodata;
    init_mm.end_rodata = (unsigned long)&_erodata;

    init_mm.start_brk = 0;
    init_mm.end_brk = memory_management_struct.end_brk;

    init_mm.start_stack = _stack_start;

    // 初始化 init_thread 和tss
    set_tss64(init_thread.rsp0, init_tss[0].rsp1, init_tss[0].rsp2, init_tss[0].ist1, init_tss[0].ist2, init_tss[0].ist3, init_tss[0].ist4, init_tss[0].ist5, init_tss[0].ist6, init_tss[0].ist7);

    init_tss[0].rsp0 = init_thread.rsp0;

    // 创建初始化thread_list
    list_init(&init_task_union.task.list);

    // 创建第二个进程(现在还是内核线程)  init
    kernel_thread(init, 10, CLONE_FS | CLONE_FILES | CLONE_SIGNAL);

    init_task_union.task.state = TASK_RUNNING;

    // 获取到init内核线程的thd 然后切换
    p = container_of(list_next(&current->list), struct task_struct, list);

    switch_to(current, p);
}
#include "interrupt.h"
#include "linkage.h"
#include "lib.h"
#include "printk.h"
#include "memory.h"
#include "gate.h"
#include "ptrace.h"

// 默认硬件
void init_8259A()
{
    int i;
    for (i = 32; i < 56; i++)
    {
        set_intr_gate(i, 2, interrupt[i - 32]);
    }

    color_printk(RED, BLACK, "8259A init \n");

    // 8259A-master	ICW1-4
    io_out8(0x20, 0x11);
    io_out8(0x21, 0x20);
    io_out8(0x21, 0x04);
    io_out8(0x21, 0x01);

    // 8259A-slave	ICW1-4
    io_out8(0xa0, 0x11);
    io_out8(0xa1, 0x28);
    io_out8(0xa1, 0x02);
    io_out8(0xa1, 0x01);

    // 8259A-M/S	OCW1
    io_out8(0x21, 0xfd); // 屏蔽除了IRQ1 也就是键盘之外的中断请求
    io_out8(0xa1, 0xff);

    sti();
}

/*
    通过rdi rsi寄存器传送参数  rsp和中断号
    统一入口
*/
void do_IRQ(struct pt_regs * regs, unsigned long nr) // regs:rsp,nr
{
    unsigned char x;
    color_printk(RED, BLACK, "do_IRQ:%#08x\t", nr);
    x = io_in8(0x60); // 键盘芯片的读写缓冲区 这里读取其中的按键扫描码
    color_printk(RED, BLACK, "key code:%#08x\n", x);
    io_out8(0x20, 0x20); // 发送EOI
    color_printk(RED, BLACK, "regs:%#018lx\t<RIP:%#018lx\tRSP:%#018lx>\n", regs, regs->rip, regs->rsp);
}

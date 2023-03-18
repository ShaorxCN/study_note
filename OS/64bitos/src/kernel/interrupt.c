#include "interrupt.h"
#include "linkage.h"
#include "lib.h"
#include "printk.h"
#include "memory.h"
#include "gate.h"
/*
    保存现场 和异常处理类似 entery.S 但是中断不会压入错误码 所以无法复用。并且都会指向同一中断处理函数(do_IRQ)
    这里比entry.S多了次pushq rax 就是补充的ERRORCODE的入栈
*/

#define SAVE_ALL             \
    "cld;			\n\t"            \
    "pushq	%rax;		\n\t"      \
    "pushq	%rax;		\n\t"      \
    "movq	%es,	%rax;	\n\t"   \
    "pushq	%rax;		\n\t"      \
    "movq	%ds,	%rax;	\n\t"   \
    "pushq	%rax;		\n\t"      \
    "xorq	%rax,	%rax;	\n\t"  \
    "pushq	%rbp;		\n\t"      \
    "pushq	%rdi;		\n\t"      \
    "pushq	%rsi;		\n\t"      \
    "pushq	%rdx;		\n\t"      \
    "pushq	%rcx;		\n\t"      \
    "pushq	%rbx;		\n\t"      \
    "pushq	%r8;		\n\t"       \
    "pushq	%r9;		\n\t"       \
    "pushq	%r10;		\n\t"      \
    "pushq	%r11;		\n\t"      \
    "pushq	%r12;		\n\t"      \
    "pushq	%r13;		\n\t"      \
    "pushq	%r14;		\n\t"      \
    "pushq	%r15;		\n\t"      \
    "movq	$0x10,	%rdx;	\n\t" \
    "movq	%rdx,	%ds;	\n\t"   \
    "movq	%rdx,	%es;	\n\t"


/*
    上面## 用于连接两个宏值。宏展开的时候它会将操作符两边的内容连接起来。 #则是吧后面的内容当作字符串
    比如Build_IRQ(0x20) 展开=> void IRQ_NAME(0x20)=>void IRQ_NAME2(IRQ##0x20）=> void IRQ0x20_interrupt(void)
    其中宏展开 先展开发现替换文本中形参是带有#或者## 则不展开实参宏 否则先展开宏参数再展开当前宏
    宏定义不是语句 不用带分号 如果带分号则分号也是替换文本 比如这里就代表是两条语句
*/
#define IRQ_NAME2(nr) nr##_interrupt(void)
#define IRQ_NAME(nr) IRQ_NAME2(IRQ##nr)

/*
    统一生成中断处理通用逻辑
    这里同理 首先执行IRQ_NAME(nr) 也就是声明函数 例如nr是0x20  void IRQ0x20_interrupt(void)
    然后是执行汇编代码生成lable 可以视为函数入口：IRQ0x20_intterrupt
    同理movq $那行 是 moveq $0x20,%rsi来传参

    ret_from_intr需要保存中断程序返回的地址 因为jmp 不会压栈 而ret需要
*/

#define Build_IRQ(nr)                                                          \
    void IRQ_NAME(nr);                                                         \
    __asm__(SYMBOL_NAME_STR(IRQ) #nr "_interrupt:		\n\t"                       \
                                     "pushq	$0x00				\n\t" SAVE_ALL \
                                     "movq	%rsp,	%rdi			\n\t"                  \
                                     "leaq	ret_from_intr(%rip),	%rax	\n\t"     \
                                     "pushq	%rax				\n\t"                      \
                                     "movq	$" #nr ",	%rsi			\n\t"             \
                                     "jmp	do_IRQ	\n\t");

/*

*/

Build_IRQ(0x20)
Build_IRQ(0x21)
Build_IRQ(0x22)
Build_IRQ(0x23)
Build_IRQ(0x24)
Build_IRQ(0x25)
Build_IRQ(0x26)
Build_IRQ(0x27)
Build_IRQ(0x28)
Build_IRQ(0x29)
Build_IRQ(0x2a)
Build_IRQ(0x2b)
Build_IRQ(0x2c)
Build_IRQ(0x2d)
Build_IRQ(0x2e)
Build_IRQ(0x2f)
Build_IRQ(0x30)
Build_IRQ(0x31)
Build_IRQ(0x32)
Build_IRQ(0x33)
Build_IRQ(0x34)
Build_IRQ(0x35)
Build_IRQ(0x36)
Build_IRQ(0x37)


// 这里说明下  x86架构下 0x00-0x13(00-19)固定异常异常  0x14-0x1F(20-31)保留 0x20-0xff(32-255)用户自定义 所以下面init_interrupt从32开始
void (*interrupt[24])(void) =
    {
        IRQ0x20_interrupt,
        IRQ0x21_interrupt,
        IRQ0x22_interrupt,
        IRQ0x23_interrupt,
        IRQ0x24_interrupt,
        IRQ0x25_interrupt,
        IRQ0x26_interrupt,
        IRQ0x27_interrupt,
        IRQ0x28_interrupt,
        IRQ0x29_interrupt,
        IRQ0x2a_interrupt,
        IRQ0x2b_interrupt,
        IRQ0x2c_interrupt,
        IRQ0x2d_interrupt,
        IRQ0x2e_interrupt,
        IRQ0x2f_interrupt,
        IRQ0x30_interrupt,
        IRQ0x31_interrupt,
        IRQ0x32_interrupt,
        IRQ0x33_interrupt,
        IRQ0x34_interrupt,
        IRQ0x35_interrupt,
        IRQ0x36_interrupt,
        IRQ0x37_interrupt,
};


// 默认硬件
void init_interrupt()
{
    int i;
    for (i = 32; i < 56; i++)
    {
        set_intr_gate(i, 2, interrupt[i - 32]);
    }

    color_printk(RED, BLACK, "8259A init \n");

    //8259A-master	ICW1-4
	io_out8(0x20,0x11);
	io_out8(0x21,0x20);
	io_out8(0x21,0x04);
	io_out8(0x21,0x01);

	//8259A-slave	ICW1-4
	io_out8(0xa0,0x11);
	io_out8(0xa1,0x28);
	io_out8(0xa1,0x02);
	io_out8(0xa1,0x01);

	//8259A-M/S	OCW1
	io_out8(0x21,0xfd);  // 屏蔽除了IRQ1 也就是键盘之外的中断请求
	io_out8(0xa1,0xff);

	sti();
}

/*
    通过rdi rsi寄存器传送参数  rsp和中断号
    统一入口
*/
void do_IRQ(unsigned long regs,unsigned long nr)	//regs:rsp,nr
{
    unsigned char x;
	color_printk(RED,BLACK,"do_IRQ:%#08x\t",nr);
	x = io_in8(0x60);  // 键盘芯片的读写缓冲区 这里读取其中的按键扫描码
	color_printk(RED,BLACK,"key code:%#08x\n",x);
	io_out8(0x20,0x20);  // 发送EOI
}

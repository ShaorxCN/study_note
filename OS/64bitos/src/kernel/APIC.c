#include "cpu.h"

void Local_APIC_init()
{
    unsigned int x, y;
    unsigned int a, b, c, d;

    // 检查是否支持apic和x2apic 参考文档中说明 主要是看edx bit[9]看是否支持 和ebx 高字节获取local apic ID ecx看是否支持xapic
    get_cpuid(1, 0, &a, &b, &c, &d);
    color_printk(WHITE, BLACK, "CPUID\t01,eax:%#010x,ebx:%#010x,ecx:%#010x,edx: %#010x\n", a, b, c, d);

    if ((1 << 9) & d)
        color_printk(WHITE, BLACK, "HW support APIC&xAPIC\t");
    else
        color_printk(WHITE, BLACK, "HW NO support APIC&xAPIC\t");

    // 检查是否支持x2apic
    if ((1 << 21) & c)
        color_printk(WHITE, BLACK, "HW support x2APIC\n");
    else
        color_printk(WHITE, BLACK, "HW NO support x2APIC\n");

    // 通过svr寄存器使能xapic和x2apic  0x1b 找到ia32_apic_base  通过rcx索引msr寄存器组
    // 然后将rax的 bit[10]和bit[11]存入cf然后将rax的bit10和11置1 这时候通过wrmsr将rax的值写入 这个bit10 和 11就是同时en和extd置位 即使开启xapic和x2apic
    __asm__ __volatile__("movq 	$0x1b,	%%rcx	\n\t"
                         "rdmsr	\n\t"
                         "bts	$10,	%%rax	\n\t"
                         "bts	$11,	%%rax	\n\t"
                         "wrmsr	\n\t"
                         "movq 	$0x1b,	%%rcx	\n\t"
                         "rdmsr	\n\t"
                         : "=a"(x), "=d"(y)
                         :
                         : "memory");
    color_printk(WHITE, BLACK, "eax:%#010x,edx:%#010x\t", x, y);
    if (x & 0xc00)
        color_printk(WHITE, BLACK, "xAPIC & x2APIC enabled\n");

    // enable SVR[8] 开启local apic 这边认为使开启x2apic 就都通过msr方式访问  80f到svr
    // bit8 开启apic  bit12 禁止EOI广播
    __asm__ __volatile__("movq 	$0x80f,	%%rcx	\n\t"
                         "rdmsr	\n\t"
                         "bts	$8,	%%rax	\n\t"
                         "bts	$12,	%%rax\n\t"
                         "wrmsr	\n\t"
                         "movq 	$0x80f,	%%rcx	\n\t"
                         "rdmsr	\n\t"
                         : "=a"(x), "=d"(y)
                         :
                         : "memory");
    color_printk(WHITE, BLACK, "eax:%#010x,edx:%#010x\t", x, y);

    if (x & 0x100)
        color_printk(WHITE, BLACK, "SVR[8] enabled\n");

    // 读取local apic id寄存器和版本寄存器

    // local apic id
    __asm__ __volatile__(
        "movq $0x802,%%rcx \n\t"
        "rdmsr \n\t"
        : "=a"(x), "=d"(y)
        :
        : "memory");

    color_printk(WHITE, BLACK, "eax:%#010x,edx:%#010x\tx2APIC ID:%#010x\n", x, y, x);

    // local apic version
    __asm__ __volatile__(
        "movq $0x803,%%rcx \n\t"
        "rdmsr \n\t"
        : "=a"(x), "=d"(y)
        :
        : "memory");

    color_printk(WHITE, BLACK, "local APIC Version:%#010x,Max LVT Entry:%#010x, SVR(Suppress EOI Broadcast):%#04x\t", x & 0xff, (x >> 16 & 0xff) + 1, x >> 24 & 0x1);

    if ((x & 0xff) < 0x10)
        color_printk(WHITE, BLACK, "82489DX discrete APIC\n");
    else if (((x & 0xff) >= 0x10) && ((x & 0xff) <= 0x15))
        color_printk(WHITE, BLACK, "Integrated APIC\n");

    // 屏蔽所有lvt中断
    __asm__ __volatile__("movq   $0x82f,    %%rcx    \n\t" // CMCI
                         "wrmsr  \n\t"
                         "movq   $0x832,    %%rcx    \n\t" // Timer
                         "wrmsr  \n\t"
                         "movq   $0x833,    %%rcx    \n\t" // Thermal Monitor
                         "wrmsr  \n\t"
                         "movq   $0x834,    %%rcx    \n\t" // Performance Counter
                         "wrmsr  \n\t"
                         "movq   $0x835,    %%rcx    \n\t" // LINT0
                         "wrmsr  \n\t"
                         "movq   $0x836,    %%rcx    \n\t" // LINT1
                         "wrmsr  \n\t"
                         "movq   $0x837,    %%rcx    \n\t" // Error
                         "wrmsr  \n\t"
                         :
                         : "a"(0x10000), "d"(0x00)
                         : "memory");

    color_printk(GREEN, BLACK, "Mask ALL LVT\n");

    // TPR
    __asm__ __volatile__("movq 	$0x808,	%%rcx	\n\t"
                         "rdmsr	\n\t"
                         : "=a"(x), "=d"(y)
                         :
                         : "memory");

    color_printk(GREEN, BLACK, "Set LVT TPR:%#010x\t", x);

    // PPR
    __asm__ __volatile__("movq 	$0x80a,	%%rcx	\n\t"
                         "rdmsr	\n\t"
                         : "=a"(x), "=d"(y)
                         :
                         : "memory");

    color_printk(GREEN, BLACK, "Set LVT PPR:%#010x\n", x);
}

void APIC_IOAPIC_init()
{
    //	init trap abort fault
    int i;

    for (i = 32; i < 56; i++)
    {
        set_intr_gate(i, 2, interrupt[i - 32]);
    }

    // mask 8259A
    color_printk(GREEN, BLACK, "MASK 8259A\n");
    io_out8(0x21, 0xff);
    io_out8(0xa1, 0xff);

    // enable IMCR 且强制转发apic
    io_out8(0x22, 0x70);
    io_out8(0x23, 0x01);

    // open IF eflages
    sti();
}
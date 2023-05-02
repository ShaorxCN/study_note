#include "cpu.h"
#include "APIC.h"
#include "lib.h"
#include "ptrace.h"
#include "interrupt.h"
#include "linkage.h"
#include "gate.h"
#include "printk.h"
#include "memory.h"

// 主要赋值 bit[16]屏蔽0 不屏蔽 其他位不变
void IOAPIC_enable(unsigned long irq)
{
    unsigned long value = 0;
    value = ioapic_rte_read((irq - 32) * 2 + 0x10);
    value = value & (~0x10000UL);
    ioapic_rte_write((irq - 32) * 2 + 0x10, value);
}

// 主要赋值 bit[16]屏蔽1 屏蔽 其他位不变
void IOAPIC_disable(unsigned long irq)
{
    unsigned long value = 0;
    value = ioapic_rte_read((irq - 32) * 2 + 0x10);
    value = value | 0x10000UL;
    ioapic_rte_write((irq - 32) * 2 + 0x10, value);
}

unsigned long IOAPIC_install(unsigned long irq, void *arg)
{
    struct IO_APIC_RET_entry *entry = (struct IO_APIC_RET_entry *)arg;
    ioapic_rte_write((irq - 32) * 2 + 0x10, *(unsigned long *)entry);

    return 1;
}

void IOAPIC_uninstall(unsigned long irq)
{
    // 赋予初始值
    ioapic_rte_write((irq - 32) * 2 + 0x10, 0x10000UL);
}

// 和下面的都是通过msr操作local apic eoi寄存器
void IOAPIC_level_ack(unsigned long irq)
{
    __asm__ __volatile__("movq	$0x00,	%%rdx	\n\t"
                         "movq	$0x00,	%%rax	\n\t"
                         "movq 	$0x80b,	%%rcx	\n\t"
                         "wrmsr	\n\t" ::
                             : "memory");

    *ioapic_map.virtual_EOI_address = irq;
}

void IOAPIC_edge_ack(unsigned long irq)
{
    __asm__ __volatile__("movq	$0x00,	%%rdx	\n\t"
                         "movq	$0x00,	%%rax	\n\t"
                         "movq 	$0x80b,	%%rcx	\n\t"
                         "wrmsr	\n\t" ::
                             : "memory");
}

// IOWIN 32bit 但是RTE64bit 所以读写操作都是两次index定位
unsigned long ioapic_rte_read(unsigned char index)
{
    unsigned long ret;

    *ioapic_map.virtual_index_address = index + 1;
    io_mfence();
    ret = *ioapic_map.virtual_data_address;
    ret <<= 32;
    io_mfence();

    *ioapic_map.virtual_index_address = index;
    io_mfence();
    ret |= *ioapic_map.virtual_data_address;
    io_mfence();

    return ret;
}

void ioapic_rte_write(unsigned char index, unsigned long value)
{
    *ioapic_map.virtual_index_address = index;
    io_mfence();
    *ioapic_map.virtual_data_address = value & 0xffffffff;
    value >>= 32;
    io_mfence();

    *ioapic_map.virtual_index_address = index + 1;
    io_mfence();
    *ioapic_map.virtual_data_address = value & 0xffffffff;
    io_mfence();
}

// 相关地址的页映射
void IOAPIC_pagetable_remap()
{
    unsigned long *tmp;
    unsigned char *IOAPIC_addr = (unsigned char *)Phy_To_Virt(0xfec00000);

    ioapic_map.physical_address = 0xfec00000;
    ioapic_map.virtual_index_address = IOAPIC_addr;
    ioapic_map.virtual_data_address = (unsigned int *)(IOAPIC_addr + 0x10);
    ioapic_map.virtual_EOI_address = (unsigned int *)(IOAPIC_addr + 0x40);

    Global_CR3 = Get_gdt();

    tmp = Phy_To_Virt(Global_CR3 + (((unsigned long)IOAPIC_addr >> PAGE_GDT_SHIFT) & 0x1ff));
    if (*tmp == 0)
    {
        unsigned long *virtual = kmalloc(PAGE_4K_SIZE, 0);
        set_mpl4t(tmp, mk_mpl4t(Virt_To_Phy(virtual), PAGE_KERNEL_GDT));
    }

    color_printk(YELLOW, BLACK, "1:%#018lx\t%#018lx\n", (unsigned long)tmp, (unsigned long)*tmp);

    tmp = Phy_To_Virt((unsigned long *)(*tmp & (~0xfffUL)) + (((unsigned long)IOAPIC_addr >> PAGE_1G_SHIFT) & 0x1ff));
    if (*tmp == 0)
    {
        unsigned long *virtual = kmalloc(PAGE_4K_SIZE, 0);
        set_pdpt(tmp, mk_pdpt(Virt_To_Phy(virtual), PAGE_KERNEL_Dir));
    }

    color_printk(YELLOW, BLACK, "2:%#018lx\t%#018lx\n", (unsigned long)tmp, (unsigned long)*tmp);

    tmp = Phy_To_Virt((unsigned long *)(*tmp & (~0xfffUL)) + (((unsigned long)IOAPIC_addr >> PAGE_2M_SHIFT) & 0x1ff));
    set_pdt(tmp, mk_pdt(ioapic_map.physical_address, PAGE_KERNEL_Page | PAGE_PWT | PAGE_PCD));

    color_printk(BLUE, BLACK, "3:%#018lx\t%#018lx\n", (unsigned long)tmp, (unsigned long)*tmp);

    color_printk(BLUE, BLACK, "ioapic_map.physical_address:%#010x\t\t\n", ioapic_map.physical_address);
    color_printk(BLUE, BLACK, "ioapic_map.virtual_address:%#018lx\t\t\n", (unsigned long)ioapic_map.virtual_index_address);

    flush_tlb();
}

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
    if (x&0x1000)
		color_printk(WHITE,BLACK,"SVR[12] enabled\n");

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

void IOAPIC_init()
{
    int i;
    //	I/O APIC
    //	I/O APIC	ID
    *ioapic_map.virtual_index_address = 0x00;
    io_mfence();
    *ioapic_map.virtual_data_address = 0x0f000000;
    io_mfence();
    color_printk(GREEN, BLACK, "Get IOAPIC ID REG:%#010x,ID:%#010x\n", *ioapic_map.virtual_data_address, *ioapic_map.virtual_data_address >> 24 & 0xf);
    io_mfence();

    //	I/O APIC	Version
    *ioapic_map.virtual_index_address = 0x01;
    io_mfence();
    color_printk(GREEN, BLACK, "Get IOAPIC Version REG:%#010x,MAX redirection enties:%#08d\n", *ioapic_map.virtual_data_address, ((*ioapic_map.virtual_data_address >> 16) & 0xff) + 1);

    // RTE 这边都屏蔽了 bit16 =1
    for (i = 0x10; i < 0x40; i += 2)
        ioapic_rte_write(i, 0x10020 + ((i - 0x10) >> 1));

    // 开启RTE1 向量号0x21 且边沿触发 参见lvt bit说明
    ioapic_rte_write(0x12, 0x21);
    color_printk(GREEN, BLACK, "I/O APIC Redirection Table Entries Set Finished.\n");
}

void APIC_IOAPIC_init()
{
    //	init trap abort fault
    int i;
    unsigned int x;
    unsigned int *p;

    IOAPIC_pagetable_remap();

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

    // init local apic
    Local_APIC_init();

    // init ioapic
    IOAPIC_init();

    // get RCBA address
    // x86 使用 0xCFC（配置数据端口）和 0xCF8（配置地址端口）来访问 PCI 端点设备。
    // 通过间接寻址的方式所引导rcba 他是pci总线的第31号LPC桥控制器组的F0h偏移地址处，即总线0的31号设备0功能的F0h偏移
    // RCBA寄存器的地址计算公式（通过I/O端口间接索引PCI总线上的设备地址）为0x80000000 | (0 << 16) | (31 << 11) | (0 << 8) | (0xF0 & 0xfc) = 0x8000f8f0
    io_out32(0xcf8, 0x8000f8f0);
    // 索引到了后读取
    x = io_in32(0xcfc);
    color_printk(RED, BLACK, "Get RCBA Address:%#010x\n", x);
    // bit[31:14]
    x = x & 0xffffc000;
    color_printk(RED, BLACK, "Get RCBA Address:%#010x\n", x);

    // get OIC address
    if (x > 0xfec00000 && x < 0xfee00000)
    {
        p = (unsigned int *)Phy_To_Virt(x + 0x31feUL);
    }

    // enable IOAPIC
    x = (*p & 0xffffff00) | 0x100;
    io_mfence();
    *p = x;
    io_mfence();

    memset(interrupt_desc, 0, sizeof(irq_desc_T) * NR_IRQS);
    // open IF eflages
    sti();
}

void do_IRQ(struct pt_regs *regs, unsigned long nr) // regs:rsp,nr
{
    irq_desc_T *irq = &interrupt_desc[nr - 32];

    if (irq->handler != NULL)
        irq->handler(nr, irq->parameter, regs);

    if (irq->controller != NULL && irq->controller->ack != NULL)
        irq->controller->ack(nr);
}
#include "APIC.h"
#include "memory.h"
#include "interrupt.h"
#include "printk.h"
#include "time.h"
#include "softirq.h"
#include "timer.h"
extern struct time time;

hw_int_controller HPET_int_controller =
    {
        .enable = IOAPIC_enable,
        .disable = IOAPIC_disable,
        .install = IOAPIC_install,
        .uninstall = IOAPIC_uninstall,
        .ack = IOAPIC_edge_ack,
};

void HPET_handler(unsigned long nr, unsigned long parameter, struct pt_regs *regs)
{
    jiffies++;

    set_softirq_status(TIMER_SIRQ);
}

void HPET_init()
{
    unsigned int *x;
    unsigned int *p;
    unsigned char *HPET_addr = (unsigned char *)Phy_To_Virt(0xfed00000); // HPTC中00默认的选择值

    struct IO_APIC_RET_entry entry;

    entry.vector = 34; // IRQ2 ioapic
    entry.deliver_mode = APIC_ICR_IOAPIC_Fixed;
    entry.dest_mode = ICR_IOAPIC_DELV_PHYSICAL;
    entry.deliver_status = APIC_ICR_IOAPIC_Idle;
    entry.polarity = APIC_IOAPIC_POLARITY_HIGH;
    entry.irr = APIC_IOAPIC_IRR_RESET;
    entry.trigger = APIC_ICR_IOAPIC_Edge;
    entry.mask = APIC_ICR_IOAPIC_Masked;
    entry.reserved = 0;

    entry.destination.physical.reserved1 = 0;
    entry.destination.physical.phy_dest = 0;
    entry.destination.physical.reserved2 = 0;

    register_irq(34, &entry, &HPET_handler, NULL, &HPET_int_controller, "HEPT");

    color_printk(RED, BLACK, "HPET - GCAP_ID:<%#018lx>\n", *(unsigned long *)HPET_addr);

    // time0 conf   edge triggered & 周期性触发(触发中断后comp会自增一开始的目标值) IRQ2
    *(unsigned long *)(HPET_addr + 0x100) = 0x004c;
    io_mfence();

    // time0_comp 1S 这里基于HPET计数周期 单位fs 也就是1e-15s
    // 我这边的精度是0x989680 就是这么多fs计数器自增一次
    *(unsigned long *)(HPET_addr + 0x108) = 100000000;
    io_mfence();

    // init MAIN_CNT & get CMOS time
    get_cmos_time(&time);
    *(unsigned long *)(HPET_addr + 0xf0) = 0;
    io_mfence();

    color_printk(RED, BLACK, "year%#010x,month:%#010x,day:%#010x,hour:%#010x,mintue:%#010x,second:%#010x\n", time.year, time.month, time.day, time.hour, time.minute, time.second);

    // gen_conf 设置3 使能置位+旧设备兼容标志位置位 即time0 向ioapic IRQ2
    *(unsigned long *)(HPET_addr + 0x10) = 3;
    io_mfence();
}
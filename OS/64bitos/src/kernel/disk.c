#include "disk.h"
#include "interrupt.h"
#include "APIC.h"
#include "memory.h"
#include "printk.h"
#include "lib.h"

hw_int_controller disk_int_controller =
    {
        .enable = IOAPIC_enable,
        .disable = IOAPIC_disable,
        .install = IOAPIC_install,
        .uninstall = IOAPIC_uninstall,
        .ack = IOAPIC_edge_ack,
};

void disk_handler(unsigned long nr, unsigned long parameter, struct pt_regs *regs)
{
    int i = 0;
    unsigned char a[512];
    // 读取数据
    port_insw(PORT_DISK1_DATA, &a, 256);
    color_printk(ORANGE, WHITE, "Read One Sector Finished:%02x\n", io_in8(PORT_DISK1_STATUS_CMD));
    for (i = 0; i < 512; i++)
        color_printk(ORANGE, WHITE, "%02x ", a[i]);
}

void disk_init()
{
    struct IO_APIC_RET_entry entry;

    // IRQ15 还是边沿触发
    entry.vector = 0x2f;
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

    register_irq(0x2f, &entry, &disk_handler, 0, &disk_int_controller, "disk1");

    // 使能 普通操作
    io_out8(PORT_DISK1_ALT_STA_CTL, 0);
    io_out8(PORT_DISK1_ERR_FEATURE, 0);
    // 操作扇区数0
    io_out8(PORT_DISK1_SECTOR_CNT, 0);
    // lba的24bit寻址
    io_out8(PORT_DISK1_SECTOR_LOW, 0);
    io_out8(PORT_DISK1_SECTOR_MID, 0);
    io_out8(PORT_DISK1_SECTOR_HIGH, 0);
    // 设备配置寄存器初始化 LBA模式
    io_out8(PORT_DISK1_DEVICE, 0xf0);
    // 识别指令
    io_out8(PORT_DISK1_STATUS_CMD, 0xec); // identify
}

void disk_exit()
{
    unregister_irq(0x2f);
}

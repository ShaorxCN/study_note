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
    // struct Disk_Identify_Info a;
    // unsigned short *p = NULL;
    unsigned char a[512];

    // color_printk(ORANGE, WHITE, "\nSerial Number:");
    // for (i = 0; i < 10; i++)
    //     color_printk(ORANGE, WHITE, "%c%c", (a.Serial_Number[i] >> 8) & 0xff, a.Serial_Number[i] & 0xff);

    // color_printk(ORANGE, WHITE, "\nFirmware revision:");
    // for (i = 0; i < 4; i++)
    //     color_printk(ORANGE, WHITE, "%c%c", (a.Firmware_Version[i] >> 8) & 0xff, a.Firmware_Version[i] & 0xff);

    // color_printk(ORANGE, WHITE, "\nModel number:");
    // for (i = 0; i < 20; i++)
    //     color_printk(ORANGE, WHITE, "%c%c", (a.Model_Number[i] >> 8) & 0xff, a.Model_Number[i] & 0xff);
    // color_printk(ORANGE, WHITE, "\n");

    // p = (unsigned short *)&a;
    // 这边读取出来的是word 记得是大端序 例如这边寻址扇区的范围是 fe00 001f 然后按照单个word大端序重新排就是  0x001ffe00个扇区
    // for (i = 0; i < 256; i++)
    //     color_printk(ORANGE, WHITE, "%04x ", *(p + i));
    while (!(io_in8(PORT_DISK1_STATUS_CMD) & DISK_STATUS_READY))
        ;
    color_printk(ORANGE, WHITE, "command finished:%02x\n", io_in8(PORT_DISK1_STATUS_CMD));
    // 当有数据请求的时候尝试读取数据
    while (io_in8(PORT_DISK1_STATUS_CMD) & DISK_STATUS_REQ)
    {
        // 读取256个word 就是512B
        port_insw(PORT_DISK1_DATA, &a, 256);
        for (i = 0; i < 512; i++)
            color_printk(ORANGE, WHITE, "%02x", a[i]);
    }
}

void disk_init()
{
    struct IO_APIC_RET_entry entry;
    unsigned char a[512];
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
    // io_out8(PORT_DISK1_ERR_FEATURE, 0);

    while ((io_in8(PORT_DISK1_STATUS_CMD) & DISK_STATUS_BUSY))
        ;
    io_out8(PORT_DISK1_DEVICE, 0x40); // 这边配置寄存器改为48bitLBA
    // 48bit的第一次操作开始
    io_out8(PORT_DISK1_ERR_FEATURE, 0); // 双次操作要求都是0
    // 操作扇区数的高8bit
    io_out8(PORT_DISK1_SECTOR_CNT, 0);

    // lba的48bit寻址的高24位
    io_out8(PORT_DISK1_SECTOR_LOW, 0);
    io_out8(PORT_DISK1_SECTOR_MID, 0);
    io_out8(PORT_DISK1_SECTOR_HIGH, 0);

    // 第二次
    io_out8(PORT_DISK1_ERR_FEATURE, 0);
    // 操作扇区数的低8bit
    io_out8(PORT_DISK1_SECTOR_CNT, 1);

    // lba的48bit寻址的低24位
    io_out8(PORT_DISK1_SECTOR_LOW, 0x12);
    io_out8(PORT_DISK1_SECTOR_MID, 0);
    io_out8(PORT_DISK1_SECTOR_HIGH, 0);

    while (!(io_in8(PORT_DISK1_STATUS_CMD) & DISK_STATUS_READY))
        ;
    color_printk(ORANGE, WHITE, "Send CMD:%02x\n", io_in8(PORT_DISK1_STATUS_CMD));
    io_out8(PORT_DISK1_STATUS_CMD, 0x34); // 48LBA write

    // 等待数据请求 也就是数据缓冲区准备就绪
    while (!(io_in8(PORT_DISK1_STATUS_CMD) & DISK_STATUS_REQ))
        ;

    memset(&a, 0xA5, 512);
    port_outsw(PORT_DISK1_DATA, &a, 256); // 数据传入

    // 等待就绪 重新28bit LBA read
    while (!(io_in8(PORT_DISK1_STATUS_CMD) & DISK_STATUS_READY))
        ;

    // 设备配置寄存器初始化 LBA模式 这边还是虚拟机 挂在ata1-master 他的主控制器设置的端口就是170 所以这边0xe0实际是主硬盘 但是是通过17x端口控制
    io_out8(PORT_DISK1_DEVICE, 0xe0);

    // 识别指令
    // io_out8(PORT_DISK1_STATUS_CMD, 0xec); // identify

    // 使用24bitLBA  读取扇区数据
    io_out8(PORT_DISK1_ERR_FEATURE, 0);
    io_out8(PORT_DISK1_SECTOR_CNT, 1);
    io_out8(PORT_DISK1_SECTOR_LOW, 0x12);
    io_out8(PORT_DISK1_SECTOR_MID, 0);
    io_out8(PORT_DISK1_SECTOR_HIGH, 0);

    while (!(io_in8(PORT_DISK1_STATUS_CMD) & DISK_STATUS_READY))
        ;
    color_printk(ORANGE, WHITE, "Send CMD:%02x\n", io_in8(PORT_DISK1_STATUS_CMD));

    io_out8(PORT_DISK1_STATUS_CMD, 0x20); ////read
}

void disk_exit()
{
    unregister_irq(0x2f);
}

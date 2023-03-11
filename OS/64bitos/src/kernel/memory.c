#include "lib.h"
#include "memory.h"

// 获取内存信息
void init_memory()
{
    int i, j;
    unsigned long TotalMem = 0;
    struct E820 *p = NULL;

    color_printk(BLUE, BLACK, "Display Physics Address MAP,Type(1:RAM,2:ROM or Reserved,3:ACPI Reclaim Memory,4:ACPI NVS Memory,Others:Undefine)\n");
    p = (struct E820 *)0xffff800000007e00;

    // 正常不是32 可能是在300多？ 参考osdev https://wiki.osdev.org/Detecting_Memory_(x86)#Getting_an_E820_Memory_Map
    // 老代码中到了32则警告返回。
    for (i = 0; i < 32; i++)
    {
        color_printk(ORANGE, BLACK, "Address:%#018lx\tLength:%#018lx\tType:%#010x\n", p->address, p->length, p->type);

        if (p->type == 1)
            TotalMem += p->length;
        memory_management_struct.e820[i].address += p->address;
        memory_management_struct.e820[i].length += p->length;
        memory_management_struct.e820[i].type = p->type;
        memory_management_struct.e820_length = i;

        p++;
        if (p->type > 4 || p->length == 0 || p->type < 1)
            break;
    }

    // tip 这里相当于带前缀的long类型16进制 宽度18 前补0
    color_printk(ORANGE, BLACK, "OS Can Used Total RAM:%#018lx\n", TotalMem);

    // 获取2m分页下的可通内存
    TotalMem = 0;
    for (i = 0; i < memory_management_struct.e820_length; i++)
    {
        unsigned long start, end;
        if (memory_management_struct.e820[i].type != 1)
            continue;
        // 向上对齐的开始地址
        start = PAGE_2M_ALIGN(memory_management_struct.e820[i].address);
        // 向下对齐的结束地址 多的截断
        end = ((memory_management_struct.e820[i].address + memory_management_struct.e820[i].length) >> PAGE_2M_SHIFT) << PAGE_2M_SHIFT;
        // 不够就算了
        if (end <= start)
            continue;
        TotalMem += (end - start) >> PAGE_2M_SHIFT;
    }
    color_printk(ORANGE, BLACK, "OS Can Used Total 2M PAGEs:%#010x=%010d\n", TotalMem, TotalMem);

    // 理论下边界再最后一个e820的边界
    TotalMem = memory_management_struct.e820[memory_management_struct.e820_length].address + memory_management_struct.e820[memory_management_struct.e820_length].length;

    // 初始化位图 内核程序结束向上对齐后的位置
    memory_management_struct.bits_map = (unsigned long)((memory_management_struct.end_brk + PAGE_4K_SIZE - 1) & PAGE_4K_MASK);
    memory_management_struct.bits_size = TotalMem >> PAGE_2M_SHIFT;
    // long 64bit 8字节。这里计算多少个2M物理页 然后按照8字节/指针大小向上对齐 计算
    memory_management_struct.bits_length = (((unsigned long)(TotalMem >> PAGE_2M_SHIFT) + sizeof(long) * 8 - 1) / 8) & (~(sizeof(long) - 1));
    memset(memory_management_struct.bits_map, 0xff, memory_management_struct.bits_length);

    // 初始化页结构指针
    memory_management_struct.pages_struct = (struct Page *)(((unsigned long)memory_management_struct.bits_map + memory_management_struct.bits_length + PAGE_4K_SIZE - 1) & PAGE_4K_MASK);
    memory_management_struct.page_size = TotalMem >> PAGE_2M_SHIFT;
    memory_management_struct.pages_length = ((TotalMem >> PAGE_2M_SHIFT) * sizeof(struct Page) + sizeof(long) - 1) & (~(sizeof(long) - 1));
    memset(memory_management_struct.pages_struct, 0x00, memory_management_struct.pages_length);

    // zone初始化
    memory_management_struct.zones_struct = (struct Zone *)(((unsigned long)memory_management_struct.pages_struct + memory_management_struct.pages_length + PAGE_4K_SIZE - 1) & PAGE_4K_MASK);
    memory_management_struct.zones_size = 0;
    memory_management_struct.zones_length = (5 * sizeof(struct Zone) + sizeof(long) - 1) & (~(sizeof(long) - 1));
    memset(memory_management_struct.zones_struct, 0x00, memory_management_struct.zones_length);

    // 根据实际内存初始化
    for (i = 0; i <= memory_management_struct.e820_length; i++)
    {
        unsigned long start, end;
        struct Zone *z;
        struct Page *p;

        unsigned long *b;

        if (memory_management_struct.e820[i].type != 1)
            continue;
        start = PAGE_2M_ALIGN(memory_management_struct.e820[i].address);
        end = ((memory_management_struct.e820[i].address + memory_management_struct.e820[i].length) >> PAGE_2M_SHIFT) << PAGE_2M_SHIFT;
        if (end <= start)
            continue;

        z = memory_management_struct.zones_struct + memory_management_struct.zones_size;
        memory_management_struct.zones_size++;

        z->zone_start_address = start;
        z->zone_end_address = end;
        z->zone_length = end - start;

        z->page_using_count = 0;
        z->page_free_count = (end - start) >> PAGE_2M_SHIFT;

        z->total_pages_link = 0;
        z->attribute = 0;
        z->GMD_struct = &memory_management_struct;

        z->pages_length = (end - start) >> PAGE_2M_SHIFT;
        // 该页的index 然后总体的页指针skip多少个  内存地址连续
        z->pages_group = (struct Page *)(memory_management_struct.pages_struct + (start >> PAGE_2M_SHIFT));

        p = z->pages_group;

        for (j = 0; j < z->pages_length; j++, p++)
        {
            p->zone_struct = z;
            p->PHY_address = start + PAGE_2M_SIZE * j;
            p->attribute = 0;

            p->reference_count = 0;

            p->age = 0;

            // 位图赋值  index>>6 得到位图index   异或 这里初始默认是0 所以异或？为啥不是or
            *(memory_management_struct.bits_map + ((p->PHY_address >> PAGE_2M_SHIFT) >> 6)) ^= 1UL << (p->PHY_address >> PAGE_2M_SHIFT) % 64;
        }
    }

    // 这里因为page 0 是包含系统的  所以需要单独初始化
    memory_management_struct.pages_struct->zone_struct = memory_management_struct.zones_struct;
    memory_management_struct.pages_struct->PHY_address = 0UL;
    memory_management_struct.pages_struct->attribute = 0;
    memory_management_struct.pages_struct->reference_count = 0;
    memory_management_struct.pages_struct->age = 0;

    memory_management_struct.zones_length = (memory_management_struct.zones_size * sizeof(struct Zone) + sizeof(long) - 1) & (~(sizeof(long) - 1));

    color_printk(ORANGE, BLACK, "bits_map:%#018lx,bits_size:%#018lx,bits_length:%#018lx\n", memory_management_struct.bits_map, memory_management_struct.bits_size, memory_management_struct.bits_length);

    color_printk(ORANGE, BLACK, "pages_struct:%#018lx,pages_size:%#018lx,pages_length:%#018lx\n", memory_management_struct.pages_struct, memory_management_struct.pages_size, memory_management_struct.pages_length);

    color_printk(ORANGE, BLACK, "zones_struct:%#018lx,zones_size:%#018lx,zones_length:%#018lx\n", memory_management_struct.zones_struct, memory_management_struct.zones_size, memory_management_struct.zones_length);

    ZONE_DMA_INDEX = 0;    // need rewrite in the future
    ZONE_NORMAL_INDEX = 0; // need rewrite in the future

    for (i = 0; i < memory_management_struct.zones_size; i++) // need rewrite in the future
    {
        struct Zone *z = memory_management_struct.zones_struct + i;
        color_printk(ORANGE, BLACK, "zone_start_address:%#018lx,zone_end_address:%#018lx,zone_length:%#018lx,pages_group:%#018lx,pages_length:%#018lx\n", z->zone_start_address, z->zone_end_address, z->zone_length, z->pages_group, z->pages_length);

        if (z->zone_start_address == 0x100000000)
            ZONE_UNMAPED_INDEX = i;
    }

    memory_management_struct.end_of_struct = (unsigned long)((unsigned long)memory_management_struct.zones_struct + memory_management_struct.zones_length + sizeof(long) * 32) & (~(sizeof(long) - 1)); ////need a blank to separate memory_management_struct

    color_printk(ORANGE, BLACK, "start_code:%#018lx,end_code:%#018lx,end_data:%#018lx,end_brk:%#018lx,end_of_struct:%#018lx\n", memory_management_struct.start_code, memory_management_struct.end_code, memory_management_struct.end_data, memory_management_struct.end_brk, memory_management_struct.end_of_struct);

    i = Virt_To_Phy(memory_management_struct.end_of_struct) >> PAGE_2M_SHIFT;

    for (j = 0; j <= i; j++)
    {
        page_init(memory_management_struct.pages_struct + j, PG_PTable_Maped | PG_Kernel_Init | PG_Active | PG_Kernel);
    }

    Global_CR3 = Get_gdt();

    color_printk(INDIGO, BLACK, "Global_CR3\t:%#018lx\n", Global_CR3);
    color_printk(INDIGO, BLACK, "*Global_CR3\t:%#018lx\n", *Phy_To_Virt(Global_CR3) & (~0xff));
    color_printk(PURPLE, BLACK, "**Global_CR3\t:%#018lx\n", *Phy_To_Virt(*Phy_To_Virt(Global_CR3) & (~0xff)) & (~0xff));

    for (i = 0; i < 10; i++)
        *(Phy_To_Virt(Global_CR3) + i) = 0UL;

    flush_tlb();
}
#include "lib.h"
#include "memory.h"

// 这里做页属性赋值
unsigned long page_init(struct Page *page, unsigned long flags)
{
    // 如果页属性 是0 说明之前没被引用过 不是得话 如果flag引用或者共享等 则只增加引用次数 包括它得zone(说明是互斥？)
    if (!page->attribute)
    {
        // 第一次使用 所以这个逻辑
        *(memory_management_struct.bits_map + ((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) |= 1UL << (page->PHY_address >> PAGE_2M_SHIFT) % 64;
        page->attribute = flags;
        page->reference_count++;
        page->zone_struct->page_using_count++;
        page->zone_struct->page_free_count--;
        page->zone_struct->total_pages_link++;
    }
    else if ((page->attribute & PG_Referenced) || (page->attribute & PG_K_Share_To_U) || (flags & PG_Referenced) || (flags & PG_K_Share_To_U))
    {
        page->attribute |= flags;
        page->reference_count++;
        page->zone_struct->total_pages_link++;
    }
    else
    {
        *(memory_management_struct.bits_map + ((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) |= 1UL << (page->PHY_address >> PAGE_2M_SHIFT) % 64;
        page->attribute |= flags;
    }
    return 0;
}

/*
    从指定zone类型zone中获取number个页 并赋属性 不能跨zone

    number: number < 64

    zone_select: zone select from dma , mapped in  pagetable , unmapped in pagetable

    page_flags: struct Page flages

*/

struct Page *alloc_pages(int zone_select, int number, unsigned long page_flags)
{
    int i;
    unsigned long page = 0;

    int zone_start = 0;
    int zone_end = 0;

    switch (zone_select)
    {
    case ZONE_DMA:
        zone_start = 0;
        zone_end = ZONE_DMA_INDEX;

        break;

    case ZONE_NORMAL:
        zone_start = ZONE_DMA_INDEX;
        zone_end = ZONE_NORMAL_INDEX;

        break;

    case ZONE_UNMAPED:
        zone_start = ZONE_UNMAPED_INDEX;
        zone_end = memory_management_struct.zones_size - 1;

        break;

    default:
        color_printk(RED, BLACK, "alloc_pages error zone_select index\n");
        return NULL;
        break;
    }

    // zone开头必定是该zone的page 0
    for (i = zone_start; i <= zone_end; i++)
    {
        struct Zone *z;
        unsigned long j;
        unsigned long start, end, length;
        unsigned long tmp;

        if ((memory_management_struct.zones_struct + i)->page_free_count < number)
            continue;

        z = memory_management_struct.zones_struct + i;
        start = z->zone_start_address >> PAGE_2M_SHIFT; // 连续无空洞的话  起始地址对应的页index
        end = z->zone_end_address >> PAGE_2M_SHIFT;
        length = z->zone_length >> PAGE_2M_SHIFT; // 该zone总归多少页

        // 位图位图检索次数 第多少页模64得出某个unsinged long中得位置 tmp就是这个bitsmap中剩下得
        tmp = 64 - start % 64;

        // 这里如果有余数说明还是那个没遍历完得map  否则就是后面那个新的map 大的层级 如果不是正好 那就第一次补全64算 后面都是完整的long
        //  按照bitsmap算 里面得k再遍历map里得每个page 但是其实就一次循环？这里最多申请64个页 内部得for直接goto return了
        for (j = start; j <= end; j += j % 64 ? tmp : 64)
        {
            unsigned long *p = memory_management_struct.bits_map + (j >> 6);
            unsigned long shift = j % 64; // 对应开始页的模64 找到单个位图中的offset
            unsigned long k;
            // 这里和外部得循环实际是看有没有连续得number个 如果第一次就分配成功 直接goto return了
            for (k = shift; k < 64 - shift; k++)
            {
                // 简化就是 !(a&b)   a：先获取64个bit位 (说明就是申请64个?)b:是申请64个吗？是的话就是全部要0 否则就是对应得1（因为左移得范围是0-63 如果64就需要单独处理） 反正就是吧需要得个数位置都置1
                // 那么如果a是0 也就是都没占用 那么a&b就是0 ！进入逻辑
                if (!(((*p >> k) | (*(p + 1) << (64 - k))) & (number == 64 ? 0xffffffffffffffffUL : ((1UL << number) - 1))))
                {
                    unsigned long l;
                    // page 是zone开始的index+实际开始的offset-1  页归属于一个zone所以j必定是某个zone index
                    // 针对整个page_struct的偏移
                    page = j + k - 1;
                    for (l = 0; l < number; l++)
                    {

                        struct Page *x = memory_management_struct.pages_struct + page + l;
                        page_init(x, page_flags);
                    }
                    goto find_free_pages;
                }
            }
        }
    }

    return NULL;

find_free_pages:

    return (struct Page *)(memory_management_struct.pages_struct + page);
}

unsigned long page_clean(struct Page *page)
{
    if (!page->attribute)
    {
        page->attribute = 0;
    }
    else if ((page->attribute & PG_Referenced) || (page->attribute & PG_K_Share_To_U))
    {
        page->reference_count--;
        page->zone_struct->total_pages_link--;
        if (!page->reference_count)
        {
            page->attribute = 0;
            page->zone_struct->page_using_count--;
            page->zone_struct->page_free_count++;
        }
    }
    else
    {
        *(memory_management_struct.bits_map + ((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) &= ~(1UL << (page->PHY_address >> PAGE_2M_SHIFT) % 64);

        page->attribute = 0;
        page->reference_count = 0;
        page->zone_struct->page_using_count--;
        page->zone_struct->page_free_count++;
        page->zone_struct->total_pages_link--;
    }
    return 0;
}

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
    memory_management_struct.bits_map = (unsigned long *)((memory_management_struct.end_brk + PAGE_4K_SIZE - 1) & PAGE_4K_MASK);
    memory_management_struct.bits_size = TotalMem >> PAGE_2M_SHIFT;
    // long 64bit 8字节。这里计算多少个2M物理页 然后按照8字节/指针大小向上对齐 计算
    memory_management_struct.bits_length = (((unsigned long)(TotalMem >> PAGE_2M_SHIFT) + sizeof(long) * 8 - 1) / 8) & (~(sizeof(long) - 1));
    memset(memory_management_struct.bits_map, 0xff, memory_management_struct.bits_length);

    // 初始化页结构指针 这里的pages_size 包括内存空洞
    memory_management_struct.pages_struct = (struct Page *)(((unsigned long)memory_management_struct.bits_map + memory_management_struct.bits_length + PAGE_4K_SIZE - 1) & PAGE_4K_MASK);
    memory_management_struct.pages_size = TotalMem >> PAGE_2M_SHIFT;
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
        // 对齐过的
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

            // 位图赋值  index>>6(一个long 64bit) 得到位图index   异或 这里初始默认是0 所以异或？为啥不是or
            *(memory_management_struct.bits_map + ((p->PHY_address >> PAGE_2M_SHIFT) >> 6)) ^= 1UL << (p->PHY_address >> PAGE_2M_SHIFT) % 64;
        }
    }

    // 这里因为page 0 是包含系统的  所以需要单独初始化 那应该置位1？
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

    // 内核程序 分页结构等本身所占的页 赋予属性 右移地板除
    i = Virt_To_Phy(memory_management_struct.end_of_struct) >> PAGE_2M_SHIFT;

    for (j = 0; j <= i; j++)
    {
        page_init(memory_management_struct.pages_struct + j, PG_PTable_Maped | PG_Kernel_Init | PG_Active | PG_Kernel);
    }

    // 获取cr3值 这里是物理地址 保存得是页目录表地址 根据header.S 是 0x0000000000101000
    // *Global_CR3 是0x0000000000102000 也就是第一个3级
    // **Global_CR3 自然是第一个2级 是0x0000000000103000
    Global_CR3 = Get_gdt();

    color_printk(INDIGO, BLACK, "Global_CR3\t:%#018lx\n", Global_CR3);
    color_printk(INDIGO, BLACK, "*Global_CR3\t:%#018lx\n", *Phy_To_Virt(Global_CR3) & (~0xff));
    color_printk(PURPLE, BLACK, "**Global_CR3\t:%#018lx\n", *Phy_To_Virt(*Phy_To_Virt(Global_CR3) & (~0xff)) & (~0xff));

    // 这里将前10表项项清零
    for (i = 0; i < 10; i++)
        *(Phy_To_Virt(Global_CR3) + i) = 0UL;

    flush_tlb();
}

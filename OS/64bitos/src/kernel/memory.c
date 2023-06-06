#include "lib.h"
#include "memory.h"
#include "printk.h"

// 这里做页属性赋值
// 简化逻辑 使之更加独立
unsigned long page_init(struct Page *page, unsigned long flags)
{

    page->attribute |= flags;

    if (!page->reference_count || (page->attribute & PG_Shared))
    {
        page->reference_count++;
        page->zone_struct->total_pages_link++;
    }

    return 1;

    // // 如果页属性 是0 说明之前没被引用过 不是得话 如果flag引用或者共享等 则只增加引用次数 包括它得zone(说明是互斥？)
    // if (!page->attribute)
    // {
    //     // 第一次使用 所以这个逻辑
    //     *(memory_management_struct.bits_map + ((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) |= 1UL << (page->PHY_address >> PAGE_2M_SHIFT) % 64;
    //     page->attribute = flags;
    //     page->reference_count++;
    //     page->zone_struct->page_using_count++;
    //     page->zone_struct->page_free_count--;
    //     page->zone_struct->total_pages_link++;
    // }
    // else if ((page->attribute & PG_Referenced) || (page->attribute & PG_K_Share_To_U) || (flags & PG_Referenced) || (flags & PG_K_Share_To_U))
    // {
    //     page->attribute |= flags;
    //     page->reference_count++;
    //     page->zone_struct->total_pages_link++;
    // }
    // else
    // {
    //     *(memory_management_struct.bits_map + ((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) |= 1UL << (page->PHY_address >> PAGE_2M_SHIFT) % 64;
    //     page->attribute |= flags;
    // }
    // return 0;
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
    unsigned long attribute = 0;

    int zone_start = 0;
    int zone_end = 0;

    if (number >= 64 || number <= 0)
    {
        color_printk(RED, BLACK, "alloc_pages() ERROR: number is invalid\n");
        return NULL;
    }

    switch (zone_select)
    {
    case ZONE_DMA:
        zone_start = 0;
        zone_end = ZONE_DMA_INDEX;
        attribute = PG_PTable_Maped;

        break;

    case ZONE_NORMAL:
        zone_start = ZONE_DMA_INDEX;
        zone_end = ZONE_NORMAL_INDEX;
        attribute = PG_PTable_Maped;
        break;

    case ZONE_UNMAPED:
        zone_start = ZONE_UNMAPED_INDEX;
        zone_end = memory_management_struct.zones_size - 1;
        attribute = 0;
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
        start = z->zone_start_address >> PAGE_2M_SHIFT; //   起始地址对应的页index
        end = z->zone_end_address >> PAGE_2M_SHIFT;
        length = z->zone_length >> PAGE_2M_SHIFT; // 该zone总共多少页

        // 位图位图检索次数 第多少页模64得出某个unsinged long中得位置 tmp就是这个bitsmap中剩下得
        tmp = 64 - start % 64;

        // 这里如果有余数说明还是那个没遍历完得map  否则就是后面那个新的map 大的层级 如果不是正好 那就第一次补全64算 后面都是完整的long
        //  按照bitsmap算 里面得k再遍历map里得每个page 但是其实就一次循环？这里最多申请64个页 内部得for直接goto return了
        for (j = start; j <= end; j += j % 64 ? tmp : 64)
        {
            unsigned long *p = memory_management_struct.bits_map + (j >> 6);
            unsigned long shift = j % 64; // 对应开始页的模64 找到单个位图中的offset
            unsigned long k;
            unsigned long num = (1UL << number) - 1; // 需要的页数bit位数量的低位变为1 然后下面的按位and 来实现判断

            for (k = shift; k < 64; k++)
            {

                // 如果k=0 那么就是*(p+1)<<64 会超出64寄存器最大位宽 根据运行模式的不同 IA-32e下 超过范围的数据将被屏蔽。 (p + 1) << 64等价于*(p + 1) << 0
                // 结果异常 所以用*p参数与计算
                /**
                 * if(!(a?(b|c:d))&e)
                 * k不等于0 则说明某个map上非头开始  右移去除shift也就占用的  左移去除高位的 一共保留64 然后是异或 都是0那就是未占用 结果0 然后&还是0 取反 符合
                 */
                if (!((k ? ((*p >> k) | (*(p + 1) << (64 - k))) : *p) & (num)))
                {
                    unsigned long l;
                    page = j + k - shift; // 等于start+上k for中的移位次数 也就是开始的index
                    for (l = 0; l < number; l++)
                    {
                        struct Page *pageptr = memory_management_struct.pages_struct + page + l;

                        *(memory_management_struct.bits_map + ((pageptr->PHY_address >> PAGE_2M_SHIFT) >> 6)) |= 1UL << (pageptr->PHY_address >> PAGE_2M_SHIFT) % 64;
                        z->page_using_count++;
                        z->page_free_count--;
                        pageptr->attribute = attribute;
                    }
                    goto find_free_pages;
                }
            }
        }
    }
    color_printk(RED, BLACK, "alloc_pages() ERROR: no page can alloc\n");
    return NULL;

find_free_pages:

    return (struct Page *)(memory_management_struct.pages_struct + page);
}

// 简化逻辑 使之更加独立
unsigned long page_clean(struct Page *page)
{

    page->reference_count--;
    page->zone_struct->total_pages_link--;

    if (!page->reference_count)
    {
        page->attribute &PG_PTable_Maped;
    }

    return 1;
    // if (!page->attribute)
    // {
    //     page->attribute = 0;
    // }
    // else if ((page->attribute & PG_Referenced) || (page->attribute & PG_K_Share_To_U))
    // {
    //     page->reference_count--;
    //     page->zone_struct->total_pages_link--;
    //     if (!page->reference_count)
    //     {
    //         page->attribute = 0;
    //         page->zone_struct->page_using_count--;
    //         page->zone_struct->page_free_count++;
    //     }
    // }
    // else
    // {
    //     *(memory_management_struct.bits_map + ((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) &= ~(1UL << (page->PHY_address >> PAGE_2M_SHIFT) % 64);

    //     page->attribute = 0;
    //     page->reference_count = 0;
    //     page->zone_struct->page_using_count--;
    //     page->zone_struct->page_free_count++;
    //     page->zone_struct->total_pages_link--;
    // }
    // return 0;
}

unsigned long get_page_attribute(struct Page *page)
{
    if (page == NULL)
    {
        color_printk(RED, BLACK, "get_page_attribute() ERROR: page == NULL\n");
        return 0;
    }
    else
    {
        return page->attribute;
    }
}

unsigned long set_page_attribute(struct Page *page, unsigned long flags)
{
    if (page == NULL)
    {
        color_printk(RED, BLACK, "set_page_attribute() ERROR: page == NULL\n");
        return 0;
    }
    else
    {
        page->attribute = flags;
        return 1;
    }
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

        unsigned long tmp = 0;

        if (p->type == 1)
            TotalMem += p->length;
        memory_management_struct.e820[i].address = p->address;
        memory_management_struct.e820[i].length = p->length;
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
        // 对齐的开始地址
        start = PAGE_2M_ALIGN(memory_management_struct.e820[i].address);
        // 对齐的结束地址 多的截断
        end = ((memory_management_struct.e820[i].address + memory_management_struct.e820[i].length) >> PAGE_2M_SHIFT) << PAGE_2M_SHIFT;
        // 不够就算了
        if (end <= start)
            continue;
        TotalMem += (end - start) >> PAGE_2M_SHIFT;
    }
    color_printk(ORANGE, BLACK, "OS Can Used Total 2M PAGEs:%#010x=%010d\n", TotalMem, TotalMem);

    // 理论下边界再最后一个e820的边界 (这里使用的是所有内存 不仅仅是可用的)
    TotalMem = memory_management_struct.e820[memory_management_struct.e820_length].address + memory_management_struct.e820[memory_management_struct.e820_length].length;

    // 初始化位图 内核程序结束向上对齐后的位置(这里使用的是所有内存 不仅仅是可用的)
    memory_management_struct.bits_map = (unsigned long *)((memory_management_struct.end_brk + PAGE_4K_SIZE - 1) & PAGE_4K_MASK);
    memory_management_struct.bits_size = TotalMem >> PAGE_2M_SHIFT;
    // long 64bit 8字节。这里计算多少个2M物理页 然后按照8字节/指针大小向上对齐 计算
    memory_management_struct.bits_length = (((unsigned long)(TotalMem >> PAGE_2M_SHIFT) + sizeof(long) * 8 - 1) / 8) & (~(sizeof(long) - 1));
    // 全部置1 相当于先占位 因为这里存储在很多不可用内存: 实际运行图 多个区域中有两个是type1 但是第一个大小小于2m 所以实际是从 0x100000 开始 然后2m向上对齐 那么实际是0x200000开始可以申请的分页
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
    set_page_attribute(memory_management_struct.pages_struct, PG_PTable_Maped | PG_Kernel_Init | PG_Kernel);
    memory_management_struct.pages_struct->reference_count = 0;
    memory_management_struct.pages_struct->age = 0;

    memory_management_struct.zones_length = (memory_management_struct.zones_size * sizeof(struct Zone) + sizeof(long) - 1) & (~(sizeof(long) - 1));

    color_printk(ORANGE, BLACK, "bits_map:%#018lx,bits_size:%#018lx,bits_length:%#018lx\n", memory_management_struct.bits_map, memory_management_struct.bits_size, memory_management_struct.bits_length);

    color_printk(ORANGE, BLACK, "pages_struct:%#018lx,pages_size:%#018lx,pages_length:%#018lx\n", memory_management_struct.pages_struct, memory_management_struct.pages_size, memory_management_struct.pages_length);

    color_printk(ORANGE, BLACK, "zones_struct:%#018lx,zones_size:%#018lx,zones_length:%#018lx\n", memory_management_struct.zones_struct, memory_management_struct.zones_size, memory_management_struct.zones_length);

    ZONE_DMA_INDEX = 0;    // need rewrite in the future
    ZONE_NORMAL_INDEX = 0; // need rewrite in the future
    ZONE_UNMAPED_INDEX = 0;

    for (i = 0; i < memory_management_struct.zones_size; i++) // need rewrite in the future
    {
        struct Zone *z = memory_management_struct.zones_struct + i;
        color_printk(ORANGE, BLACK, "zone_start_address:%#018lx,zone_end_address:%#018lx,zone_length:%#018lx,pages_group:%#018lx,pages_length:%#018lx\n", z->zone_start_address, z->zone_end_address, z->zone_length, z->pages_group, z->pages_length);

        //  这边我没到4G 因为还是用的bochs 只能模拟2g
        if (z->zone_start_address == 0x100000000 && !ZONE_UNMAPED_INDEX)
            ZONE_UNMAPED_INDEX = i;
    }

    color_printk(ORANGE, BLACK, "ZONE_DMA_INDEX:%d\tZONE_NORMAL_INDEX:%d\tZONE_UNMAPED_INDEX:%d\n", ZONE_DMA_INDEX, ZONE_NORMAL_INDEX, ZONE_UNMAPED_INDEX);

    memory_management_struct.end_of_struct = (unsigned long)((unsigned long)memory_management_struct.zones_struct + memory_management_struct.zones_length + sizeof(long) * 32) & (~(sizeof(long) - 1)); ////need a blank to separate memory_management_struct

    color_printk(ORANGE, BLACK, "start_code:%#018lx,end_code:%#018lx,end_data:%#018lx,end_brk:%#018lx,end_of_struct:%#018lx\n", memory_management_struct.start_code, memory_management_struct.end_code, memory_management_struct.end_data, memory_management_struct.end_brk, memory_management_struct.end_of_struct);

    // 内核程序 分页结构等本身所占的页 赋予属性 右移地板除
    i = Virt_To_Phy(memory_management_struct.end_of_struct) >> PAGE_2M_SHIFT;

    for (j = 0; j <= i; j++)
    {
        struct Page *tmp_page = memory_management_struct.pages_struct + j;
        page_init(tmp_page, PG_PTable_Maped | PG_Kernel_Init | PG_Kernel);
        *(memory_management_struct.bits_map + ((tmp_page->PHY_address >> PAGE_2M_SHIFT) >> 6)) |= 1UL << (tmp_page->PHY_address >> PAGE_2M_SHIFT) % 64;
        tmp_page->zone_struct->page_using_count++;
        tmp_page->zone_struct->page_free_count--;
    }

    // 获取cr3值 这里是物理地址 保存得是页目录表地址 根据header.S 是 0x0000000000101000
    // *Global_CR3 是0x0000000000102000 也就是第一个3级
    // **Global_CR3 自然是第一个2级 是0x0000000000103000
    Global_CR3 = Get_gdt();

    color_printk(INDIGO, BLACK, "Global_CR3\t:%#018lx\n", Global_CR3);
    color_printk(INDIGO, BLACK, "*Global_CR3\t:%#018lx\n", *Phy_To_Virt(Global_CR3) & (~0xff));
    color_printk(PURPLE, BLACK, "**Global_CR3\t:%#018lx\n", *Phy_To_Virt(*Phy_To_Virt(Global_CR3) & (~0xff)) & (~0xff));

    color_printk(ORANGE, BLACK, "1.memory_management_struct.bits_map:%#018lx\tzone_struct->page_using_count:%d\tzone_struct->page_free_count:%d\n", *memory_management_struct.bits_map, memory_management_struct.zones_struct->page_using_count, memory_management_struct.zones_struct->page_free_count);

    // 将前10页清零 也就是清除线性地址0和0xffff800000000000在页表中的重映射(只保留 0xffff800000000000) 但是这样task_init就无法执行了
    // 设计只需需要清1个 header.S中只有index 0 和 256 设置了
    // 再次放开 因为apu的初始化用到
    // for (i = 0; i < 10; i++)
    //     *(Phy_To_Virt(Global_CR3) + i) = 0UL;

    flush_tlb();
}

// 从指定页开始释放number个页面
void free_pages(struct Page *page, int number)
{
    int i = 0;
    if (page == NULL)
    {
        color_printk(RED, BLACK, "free_pages() ERRPR: page is invalid\n");
        return;
    }

    if (number >= 64 || number <= 0)
    {
        color_printk(RED, BLACK, "free_pages() ERROR: number is invalid\n");
        return;
    }

    for (i = 0; i < number; i++, page++)
    {
        // 对应的bit位复位
        *(memory_management_struct.bits_map + ((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) &= ~(1UL << (page->PHY_address >> PAGE_2M_SHIFT) % 64);
        page->zone_struct->page_using_count--;
        page->zone_struct->page_free_count++;
        page->attribute = 0;
    }
}

//  创建 slab
struct Slab *kmalloc_create(unsigned long size)
{
    int i;
    struct Slab *slab = NULL;
    struct Page *page = NULL;
    unsigned long *vaddresss = NULL;
    long structsize = 0;

    page = alloc_pages(ZONE_NORMAL, 1, 0);

    if (page == NULL)
    {
        color_printk(RED, BLACK, "kmalloc_create()->alloc_pages()=>page == NULL\n");
        return NULL;
    }

    page_init(page, PG_Kernel);

    switch (size)
    {
        ////////////////////这个尺寸将slab和color_map就存储在该页面上

    case 32:
    case 64:
    case 128:
    case 256:
    case 512:

        vaddresss = Phy_To_Virt(page->PHY_address);
        structsize = sizeof(struct Slab) + PAGE_2M_SIZE / size / 8; // slab结构体大小加color_map大小

        // slab以及map存储在页尾部
        slab = (struct Slab *)((unsigned char *)vaddresss + PAGE_2M_SIZE - structsize);
        slab->color_map = (unsigned long *)((unsigned char *)slab + sizeof(struct Slab));

        slab->free_count = (PAGE_2M_SIZE - (PAGE_2M_SIZE / size / 8) - sizeof(struct Slab)) / size;
        slab->using_count = 0;
        slab->color_count = slab->free_count;
        slab->Vaddress = vaddresss;
        slab->page = page;
        list_init(&slab->list);

        // 这个length是字节计算的  所以是按照count 根据unsigned long的长度(64寻址单位下是8B)乘以8bit -1右移来实现向上取整  得到需要多少个unsigned long  最后有个乘以8 是字节数量(所以默认是8 byte ul)
        slab->color_length = ((slab->color_count + sizeof(unsigned long) * 8 - 1) >> 6) << 3;
        memset(slab->color_map, 0xff, slab->color_length);

        for (i = 0; i < slab->color_count; i++)
            *(slab->color_map + (i >> 6)) ^= 1UL << i % 64;

        break;

        ///////////////////kmalloc slab and map,not in 2M page anymore

    case 1024: // 1KB
    case 2048:
    case 4096: // 4KB
    case 8192:
    case 16384:

        //////////////////color_map is a very short buffer.这边因为尺寸变大了 如果还是放在同一个page中 会造成浪费 比如1个2m页
        // 但是尺寸是1m那么只能容纳一个块 因为剩下的如果存放slab colormap等的话

    case 32768:
    case 65536:
    case 131072: // 128KB
    case 262144:
    case 524288:
    case 1048576: // 1MB

        slab = (struct Slab *)kmalloc(sizeof(struct Slab), 0);

        slab->free_count = PAGE_2M_SIZE / size;
        slab->using_count = 0;
        slab->color_count = slab->free_count;

        slab->color_length = ((slab->color_count + sizeof(unsigned long) * 8 - 1) >> 6) << 3;

        slab->color_map = (unsigned long *)kmalloc(slab->color_length, 0);
        memset(slab->color_map, 0xff, slab->color_length);

        slab->Vaddress = Phy_To_Virt(page->PHY_address);
        slab->page = page;
        list_init(&slab->list);

        for (i = 0; i < slab->color_count; i++)
            *(slab->color_map + (i >> 6)) ^= 1UL << i % 64;

        break;

    default:

        color_printk(RED, BLACK, "kmalloc_create() ERROR: wrong size:%08d\n", size);
        free_pages(page, 1);

        return NULL;
    }

    return slab;
}

// 内核内存块申请(从slab中申请) size是代表的块大小 参考kmalloc_cache_size 用于申请小于1页的内存
// size 不能大于1MB  且位 2^N次方 返回void *
// 通用内存申请函数
void *kmalloc(unsigned long size, unsigned long gfg_flages)
{
    int i, j;
    struct Slab *slab = NULL;
    if (size > 1048576)
    {
        color_printk(RED, BLACK, "kmalloc() ERROR: kmalloc size too long:%08d\n", size);
        return NULL;
    }

    // 查找现有的尺寸满足的slab_cache
    for (i = 0; i < 16; i++)
        if (kmalloc_cache_size[i].size >= size)
            break;
    slab = kmalloc_cache_size[i].cache_pool;

    if (kmalloc_cache_size[i].total_free != 0)
    {
        do
        {
            if (slab->free_count == 0)
                slab = container_of(list_next(&slab->list), struct Slab, list);
            else
                break;
        } while (slab != kmalloc_cache_size[i].cache_pool); // 进来的时候是相等的 如果进来的不满足 那么就是往下
    }
    else
    {
        // 如果没有找到适合的 那就创建新的slab
        slab = kmalloc_create(kmalloc_cache_size[i].size);

        if (slab == NULL)
        {
            color_printk(BLUE, BLACK, "kmalloc()->kmalloc_create()=>slab == NULL\n");
            return NULL;
        }

        kmalloc_cache_size[i].total_free += slab->color_count;

        color_printk(BLUE, BLACK, "kmalloc()->kmalloc_create()<=size:%#010x\n", kmalloc_cache_size[i].size); ///////

        list_add_to_before(&kmalloc_cache_size[i].cache_pool->list, &slab->list);
    }

    for (j = 0; j < slab->color_count; j++)
    {
        if (*(slab->color_map + (j >> 6)) == 0xffffffffffffffffUL)
        {
            j += 63;
            continue;
        }

        if ((*(slab->color_map + (j >> 6)) & (1UL << (j % 64))) == 0)
        {
            *(slab->color_map + (j >> 6)) |= 1UL << (j % 64);
            slab->using_count++;
            slab->free_count--;

            kmalloc_cache_size[i].total_free--;
            kmalloc_cache_size[i].total_using++;

            return (void *)((char *)slab->Vaddress + kmalloc_cache_size[i].size * j);
        }
    }

    color_printk(BLUE, BLACK, "kmalloc() ERROR: no memory can alloc\n");
    return NULL;
}

// 根据地址释放修改对应的页以及slab属性 通用内存释放程序
unsigned long kfree(void *address)
{
    int i;
    int index;

    struct Slab *slab = NULL;
    // 获取页地址
    void *page_base_address = (void *)((unsigned long)address & PAGE_2M_MASK);

    // 尝试去找是哪个slab_cache的
    for (i = 0; i < 16; i++)
    {
        slab = kmalloc_cache_size[i].cache_pool;
        do
        {
            // 命中后
            if (slab->Vaddress == page_base_address)
            {

                // 找到是第几个块
                index = (address - slab->Vaddress) / kmalloc_cache_size[i].size;

                // 64bit一个ul 然后对应index的余数复位应该 释放原来肯定是1了
                *(slab->color_map + (index >> 6)) ^= 1UL << index % 64;

                slab->free_count++;
                slab->using_count--;

                kmalloc_cache_size[i].total_free++;
                kmalloc_cache_size[i].total_using--;

                // 当前slab空闲 整个cache的空闲数量大于该slab可用数量的1.5X  并且该slab不是手动创建的第一个静态slab 那么考虑回收释放
                if ((slab->using_count == 0) && (kmalloc_cache_size[i].total_free >= slab->color_count * 3 / 2) && (kmalloc_cache_size[i].cache_pool != slab))
                {
                    switch (kmalloc_cache_size[i].size)
                    {
                        ////////////////////slab + map in 2M page

                    case 32:
                    case 64:
                    case 128:
                    case 256:
                    case 512:
                        list_del(&slab->list);
                        kmalloc_cache_size[i].total_free -= slab->color_count;

                        page_clean(slab->page);
                        free_pages(slab->page, 1);
                        break;
                    // 这边map以及slab在其他slab 所以需要重新kfree(map) kfree(slab)
                    default:
                        list_del(&slab->list);
                        kmalloc_cache_size[i].total_free -= slab->color_count;

                        kfree(slab->color_map);

                        page_clean(slab->page);
                        free_pages(slab->page, 1);
                        kfree(slab);
                        break;
                    }
                }

                return 1;
            }
            else
                slab = container_of(list_next(&slab->list), struct Slab, list); // 未命中则是找下一个slab node

        } while (slab != kmalloc_cache_size[i].cache_pool);
    }

    color_printk(RED, BLACK, "kfree() ERROR: can`t free memory\n");

    return 0;
}
// 创建slab_cache
struct Slab_cache *slab_create(unsigned long size, void *(*constructor)(void *Vaddress, unsigned long arg), void *(*destructor)(void *Vaddress, unsigned long arg), unsigned long arg)
{
    struct Slab_cache *slab_cache = NULL;
    slab_cache = (struct Slab_cache *)kmalloc(sizeof(struct Slab_cache), 0);

    if (slab_cache == NULL)
    {
        color_printk(RED, BLACK, "slab_create()->kmalloc()=>slab_cache == NULL\n");
        return NULL;
    }

    memset(slab_cache, 0, sizeof(struct Slab_cache));

    slab_cache->size = SIZEOF_LONG_ALIGN(size);
    slab_cache->total_using = 0;
    slab_cache->total_free = 0;
    slab_cache->cache_pool = (struct Slab *)kmalloc(sizeof(struct Slab), 0);

    if (slab_cache->cache_pool == NULL)
    {
        color_printk(RED, BLACK, "slab_create()->kmalloc()=>slab_cache->cache_pool == NULL\n");
        kfree(slab_cache);
        return NULL;
    }

    memset(slab_cache->cache_pool, 0, sizeof(struct Slab));

    slab_cache->cache_dma_pool = NULL;
    slab_cache->constructor = constructor;
    slab_cache->destructor = destructor;
    list_init(&slab_cache->cache_pool->list);

    slab_cache->cache_pool->page = alloc_pages(ZONE_NORMAL, 1, 0);
    if (slab_cache->cache_pool->page == NULL)
    {
        color_printk(RED, BLACK, "slab_create()->alloc_pages()=>slab_cache->cache_pool->page == NULL\n");
        kfree(slab_cache->cache_pool);
        kfree(slab_cache);
        return NULL;
    }

    page_init(slab_cache->cache_pool->page, PG_Kernel);

    slab_cache->cache_pool->using_count = 0;
    slab_cache->cache_pool->free_count = PAGE_2M_SIZE / slab_cache->size;
    slab_cache->total_free = slab_cache->cache_pool->free_count;
    slab_cache->cache_pool->Vaddress = Phy_To_Virt(slab_cache->cache_pool->page);
    slab_cache->cache_pool->color_count = slab_cache->cache_pool->free_count;
    // 64一个单位 所以按照count+64-1 右移6向下对齐 然后左移3算到字节为单位
    slab_cache->cache_pool->color_length = ((slab_cache->cache_pool->color_count + sizeof(unsigned long) * 8 - 1) >> 6) << 3;

    slab_cache->cache_pool->color_map = (unsigned long *)kmalloc(slab_cache->cache_pool->color_length, 0);
    if (slab_cache->cache_pool->color_map == NULL)
    {
        color_printk(RED, BLACK, "slab_create()->kmalloc()=>slab_cache->cache_pool->color_map == NULL\n");
        free_pages(slab_cache->cache_pool->page, 1);
        kfree(slab_cache->cache_pool);
        kfree(slab_cache);
        return NULL;
    }

    memset(slab_cache->cache_pool->color_map, 0, slab_cache->cache_pool->color_length);

    return slab_cache;
}

// 释放slab_cache 首先没有占用的  然后一次释放所有list slab(包括里面的两个引用page 和 map)   最后再释放slab_cache
unsigned long slab_destroy(struct Slab_cache *slab_cache)
{
    struct Slab *slab_p = slab_cache->cache_pool;
    struct Slab *tmp_slab = NULL;

    if (slab_cache->total_using != 0)
    {
        color_printk(RED, BLACK, "slab_cache->total_using != 0\n");
        return 0;
    }

    while (!list_is_empty(&slab_p->list))
    {
        tmp_slab = slab_p;
        slab_p = container_of(list_next(&slab_p->list), struct Slab, list);

        list_del(&tmp_slab->list);
        kfree(tmp_slab->color_map);

        page_clean(tmp_slab->page);
        free_pages(tmp_slab->page, 1);
        kfree(tmp_slab);
    }

    kfree(slab_p->color_map);
    page_clean(slab_p->page);
    free_pages(slab_p->page, 1);
    kfree(slab_p);
    kfree(slab_cache);

    return 1;
}

// 从指定slab_cache 中申请一个size内存单位
void *slab_malloc(struct Slab_cache *slab_cache, unsigned long arg)
{
    struct Slab *slab_p = slab_cache->cache_pool;
    struct Slab *tmp_slab = NULL;
    int j = 0;

    // 已经用完 申请新的slab
    if (slab_cache->total_free == 0)
    {
        tmp_slab = (struct Slab *)kmalloc(sizeof(struct Slab), 0);

        if (tmp_slab == NULL)
        {
            color_printk(RED, BLACK, "slab_malloc()->kmalloc()=>tmp_slab == NULL\n");
            return NULL;
        }

        memset(tmp_slab, 0, sizeof(struct Slab));
        list_init(&tmp_slab->list);

        tmp_slab->page = alloc_pages(ZONE_NORMAL, 1, 0);
        if (tmp_slab->page == NULL)
        {
            color_printk(RED, BLACK, "slab_malloc()->alloc_pages()=>tmp_slab->page == NULL\n");
            kfree(tmp_slab);
            return NULL;
        }

        page_init(tmp_slab->page, PG_Kernel);
        tmp_slab->using_count = PAGE_2M_SIZE / slab_cache->size;
        tmp_slab->free_count = tmp_slab->using_count;
        tmp_slab->Vaddress = Phy_To_Virt(tmp_slab->page->PHY_address);

        tmp_slab->color_count = tmp_slab->free_count;
        tmp_slab->color_length = ((tmp_slab->color_count + sizeof(unsigned long) * 8 - 1) >> 6) << 3;
        tmp_slab->color_map = (unsigned long *)kmalloc(tmp_slab->color_length, 0);

        if (tmp_slab->color_map == NULL)
        {
            color_printk(RED, BLACK, "slab_malloc()->kmalloc()=>tmp_slab->color_map == NULL\n");
            free_pages(tmp_slab->page, 1);
            kfree(tmp_slab);
            return NULL;
        }

        memset(tmp_slab->color_map, 0, tmp_slab->color_length);
        list_add_to_behind(&slab_cache->cache_pool->list, &tmp_slab->list);
        slab_cache->total_free += tmp_slab->color_count;

        for (j = 0; j < tmp_slab->color_count; j++)
        {
            // 这个有必要吗 新建逻辑中的的然后memset的是0 都是空闲的
            if ((*(tmp_slab->color_map + (j >> 6)) & (1UL << (j % 64))) == 0)
            {
                // 置位
                *(tmp_slab->color_map + (j >> 6)) |= 1UL << (j % 64);

                tmp_slab->using_count++;
                tmp_slab->free_count--;

                slab_cache->total_using++;
                slab_cache->total_free--;

                if (slab_cache->constructor != NULL)
                {
                    return slab_cache->constructor((char *)tmp_slab->Vaddress + slab_cache->size * j, arg);
                }
                else
                {
                    return (void *)((char *)tmp_slab->Vaddress + slab_cache->size * j);
                }
            }
        }
    }
    else
    {
        // 这边就从现存中的找
        do
        {
            if (slab_p->free_count == 0)
            {
                slab_p = container_of(list_next(&slab_p->list), struct Slab, list);
                continue;
            }

            for (j = 0; j < slab_p->color_count; j++)
            {

                if (*(slab_p->color_map + (j >> 6)) == 0xffffffffffffffffUL)
                {
                    j += 63;
                    continue;
                }

                if ((*(slab_p->color_map + (j >> 6)) & (1UL << (j % 64))) == 0)
                {
                    *(slab_p->color_map + (j >> 6)) |= 1UL << (j % 64);

                    slab_p->using_count++;
                    slab_p->free_count--;

                    slab_cache->total_using++;
                    slab_cache->total_free--;

                    if (slab_cache->constructor != NULL)
                    {
                        return slab_cache->constructor((char *)slab_p->Vaddress + slab_cache->size * j, arg);
                    }
                    else
                    {
                        return (void *)((char *)slab_p->Vaddress + slab_cache->size * j);
                    }
                }
            }
        } while (slab_p != slab_cache->cache_pool);
    }

    color_printk(RED, BLACK, "slab_malloc() ERROR: can`t alloc\n");
    if (tmp_slab != NULL)
    {
        list_del(&tmp_slab->list);
        kfree(tmp_slab->color_map);
        page_clean(tmp_slab->page);
        free_pages(tmp_slab->page, 1);
        kfree(tmp_slab);
    }

    return NULL;
}

// 指定slab_cache 释放指定地址的内存块
unsigned long slab_free(struct Slab_cache *slab_cache, void *address, unsigned long arg)
{
    struct Slab *slab_p = slab_cache->cache_pool;
    int index = 0;

    do
    {
        // 在该page内
        if (slab_p->Vaddress <= address && address < slab_p->Vaddress + PAGE_2M_SIZE)
        {
            index = (address - slab_p->Vaddress) / slab_cache->size;
            *(slab_p->color_map + (index >> 6)) ^= 1UL << index % 64;

            slab_cache->total_using--;
            slab_cache->total_free++;

            if (slab_cache->destructor != NULL)
            {
                //  GNU 中则允许，因为在默认情况下，GNU 认为 void * 和 char * 一样 这样来做计算操作 即以字节为单位
                slab_cache->destructor((char *)slab_p->Vaddress + slab_cache->size * index, arg);
            }

            if ((slab_p->using_count == 0) && (slab_cache->total_free >= slab_p->color_count * 3 / 2))
            {
                list_del(&slab_p->list);
                slab_cache->total_free -= slab_p->color_count;

                kfree(slab_p->color_map);

                page_clean(slab_p->page);
                free_pages(slab_p->page, 1);
                kfree(slab_p);
            }

            return 1;
        }
        else
        {
            // 查找下一个slab
            slab_p = container_of(list_next(&slab_p->list), struct Slab, list);
            continue;
        }
    } while (slab_p != slab_cache->cache_pool);

    color_printk(RED, BLACK, "slab_free() ERROR: address not in slab\n");

    return 0;
}

// 手动创建基础的slab_cache 每个先创建一个slab  这里手动计算页地址 然后返回来去计算以及设置页属性 其实可以通过alloc_pages函数
// 这里为了保证此时分配的内存页连续排列在扩展空间之后
unsigned long slab_init()
{
    struct Page *page = NULL;
    unsigned long *virtual = NULL; // get a free page and set to empty page table and return the virtual address
    unsigned long i, j;

    unsigned long tmp_address = memory_management_struct.end_of_struct;

    for (i = 0; i < 16; i++)
    {
        kmalloc_cache_size[i].cache_pool = (struct Slab *)memory_management_struct.end_of_struct;
        memory_management_struct.end_of_struct = memory_management_struct.end_of_struct + sizeof(struct Slab) + sizeof(long) * 10;

        list_init(&kmalloc_cache_size[i].cache_pool->list);

        //////////// init sizeof struct Slab of cache size

        kmalloc_cache_size[i].cache_pool->using_count = 0;
        kmalloc_cache_size[i].cache_pool->free_count = PAGE_2M_SIZE / kmalloc_cache_size[i].size;

        kmalloc_cache_size[i].cache_pool->color_length = ((PAGE_2M_SIZE / kmalloc_cache_size[i].size + sizeof(unsigned long) * 8 - 1) >> 6) << 3;

        kmalloc_cache_size[i].cache_pool->color_count = kmalloc_cache_size[i].cache_pool->free_count;

        kmalloc_cache_size[i].cache_pool->color_map = (unsigned long *)memory_management_struct.end_of_struct;

        memory_management_struct.end_of_struct = (unsigned long)(memory_management_struct.end_of_struct + kmalloc_cache_size[i].cache_pool->color_length + sizeof(long) * 10) & (~(sizeof(long) - 1));

        memset(kmalloc_cache_size[i].cache_pool->color_map, 0xff, kmalloc_cache_size[i].cache_pool->color_length);

        for (j = 0; j < kmalloc_cache_size[i].cache_pool->color_count; j++)
            *(kmalloc_cache_size[i].cache_pool->color_map + (j >> 6)) ^= 1UL << j % 64;

        kmalloc_cache_size[i].total_free = kmalloc_cache_size[i].cache_pool->color_count;
        kmalloc_cache_size[i].total_using = 0;
    }

    ////////////	init page for kernel code and memory management struct

    // 重新计算内核代码占用的页以及更新对应属性
    i = Virt_To_Phy(memory_management_struct.end_of_struct) >> PAGE_2M_SHIFT;

    for (j = PAGE_2M_ALIGN(Virt_To_Phy(tmp_address)) >> PAGE_2M_SHIFT; j <= i; j++)
    {
        page = memory_management_struct.pages_struct + j;
        *(memory_management_struct.bits_map + ((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) |= 1UL << (page->PHY_address >> PAGE_2M_SHIFT) % 64;
        page->zone_struct->page_using_count++;
        page->zone_struct->page_free_count--;
        page_init(page, PG_PTable_Maped | PG_Kernel_Init | PG_Kernel);
    }

    color_printk(ORANGE, BLACK, "2.memory_management_struct.bits_map:%#018lx\tzone_struct->page_using_count:%d\tzone_struct->page_free_count:%d\n", *memory_management_struct.bits_map, memory_management_struct.zones_struct->page_using_count, memory_management_struct.zones_struct->page_free_count);

    // 为slab_cache 分配页
    for (i = 0; i < 16; i++)
    {
        virtual = (unsigned long *)((memory_management_struct.end_of_struct + PAGE_2M_SIZE * i + PAGE_2M_SIZE - 1) & PAGE_2M_MASK);
        page = Virt_To_2M_Page(virtual);

        *(memory_management_struct.bits_map + ((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) |= 1UL << (page->PHY_address >> PAGE_2M_SHIFT) % 64;
        page->zone_struct->page_using_count++;
        page->zone_struct->page_free_count--;

        page_init(page, PG_PTable_Maped | PG_Kernel_Init | PG_Kernel);

        kmalloc_cache_size[i].cache_pool->page = page;
        kmalloc_cache_size[i].cache_pool->Vaddress = virtual;
    }

    color_printk(ORANGE, BLACK, "3.memory_management_struct.bits_map:%#018lx\tzone_struct->page_using_count:%d\tzone_struct->page_free_count:%d\n", *memory_management_struct.bits_map, memory_management_struct.zones_struct->page_using_count, memory_management_struct.zones_struct->page_free_count);

    color_printk(ORANGE, BLACK, "start_code:%#018lx,end_code:%#018lx,end_data:%#018lx,end_brk:%#018lx,end_of_struct:%#018lx\n", memory_management_struct.start_code, memory_management_struct.end_code, memory_management_struct.end_data, memory_management_struct.end_brk, memory_management_struct.end_of_struct);

    return 1;
}

void pagetable_init()
{
    unsigned long i, j;
    unsigned long *tmp = NULL;

    Global_CR3 = Get_gdt();

    // 屏蔽cr3中的标志位 然后偏移256个项(就是这个0xffff800000000000开始的 4级页表位置) 在转换为线性地址
    tmp = (unsigned long *)(((unsigned long)Phy_To_Virt((unsigned long)Global_CR3 & (~0xfffUL))) + 8 * 256);
    // 0xffff800000101800=lds的 0xffff800000000000 + 0x100000+org的1000+256个表项的8byte(800)然后值102023 这个多的2是属性accessed 这边是访问了 所以多了这个标志位 bit[5]
    color_printk(YELLOW, BLACK, "1:%#018lx,%#018lx\t\t\n", (unsigned long)tmp, *tmp);

    // 获得3j页表的线性地址
    tmp = Phy_To_Virt(*tmp & (~0xfffUL));
    // 0xffff800000102000   0x103023
    color_printk(YELLOW, BLACK, "2:%#018lx,%#018lx\t\t\n", (unsigned long)tmp, *tmp);

    // 获得pde的线性地址
    tmp = Phy_To_Virt(*tmp & (~0xfffUL));
    //  0xffff800000103000  00000000e3 设置的值是00000083 同理bit[5]=1 然后bit[6]avalibale =1 bit 还有原本的设置bit[7]PS=1 表示2M
    color_printk(YELLOW, BLACK, "3:%#018lx,%#018lx\t\t\n", (unsigned long)tmp, *tmp);

    // 将4G以内的重新映射
    for (i = 0; i < memory_management_struct.zones_size; i++)
    {
        struct Zone *z = memory_management_struct.zones_struct + i;
        struct Page *p = z->pages_group;

        // 这边不会进 模拟器还是2G init_memory中的逻辑未生效
        if (ZONE_UNMAPED_INDEX && i == ZONE_UNMAPED_INDEX)
            break;

        for (j = 0; j < z->pages_length; j++, p++)
        {
            // l4的线性地址 加上 线性地址中的lv4index*8 也就是offset 得出对应的l4表项地址
            tmp = (unsigned long *)(((unsigned long)Phy_To_Virt((unsigned long)Global_CR3 & (~0xfffUL))) + (((unsigned long)Phy_To_Virt(p->PHY_address) >> PAGE_GDT_SHIFT) & 0x1ff) * 8);

            // 如果是0则说明未映射l3页表  则申请一个4kpage 做存储次级页表使用 然后设置
            if (*tmp == 0)
            {
                unsigned long *virtual = kmalloc(PAGE_4K_SIZE, 0);
                set_mpl4t(tmp, mk_mpl4t(Virt_To_Phy(virtual), PAGE_KERNEL_GDT));
            }

            // 同理检查l3的index 没有则继续申请并设置
            tmp = (unsigned long *)((unsigned long)Phy_To_Virt(*tmp & (~0xfffUL)) + (((unsigned long)Phy_To_Virt(p->PHY_address) >> PAGE_1G_SHIFT) & 0x1ff) * 8);

            if (*tmp == 0)
            {
                unsigned long *virtual = kmalloc(PAGE_4K_SIZE, 0);
                set_pdpt(tmp, mk_pdpt(Virt_To_Phy(virtual), PAGE_KERNEL_Dir));
            }

            // 最后就是l2的线性地址 借助线性地址设置 这里是2M page这里就结束 设置pde的物理地址
            tmp = (unsigned long *)((unsigned long)Phy_To_Virt(*tmp & (~0xfffUL)) + (((unsigned long)Phy_To_Virt(p->PHY_address) >> PAGE_2M_SHIFT) & 0x1ff) * 8);

            set_pdt(tmp, mk_pdt(p->PHY_address, PAGE_KERNEL_Page));

            // 每50page 输出检查 也就是i物理地址相差100M  0x6400000 线性地址同表相差8*50=400B 0x190
            // if (j % 50 == 0)
            //     color_printk(GREEN, BLACK, "@:%#018lx,%#018lx\t\n", (unsigned long)tmp, *tmp);
        }
    }

    flush_tlb();
}
#include "lib.h"
#include "gate.h"
#include "trap.h"
#include "printk.h"
#include "memory.h"
#include "interrupt.h"
#include "task.h"
#include "cpu.h"

#if APIC
#include "APIC.h"
#include "keyboard.h"
#include "mouse.h"
#include "disk.h"
#else
#include "8259A.h"
#endif

// 段标识符 代码段开始 代码段结束 数据段结束 程序结束 在Kernel.lds中有指明
extern char _text;
extern char _etext;
extern char _edata;
extern char _end;

struct Global_Memory_Descriptor memory_management_struct = {{0}, 0};

void Start_Kernel(void)
{
	// header.S 中将帧缓存的物理地址(0xe0000000) 映射到0xffff800000a00000和0xa00000处
	// loader设置了显示模式(（模式号：0x180、分辨率：1440×900、颜色深度：32 bit）)
	// Loader引导加载程序设置的显示模式可支持32位颜色深度的像素点，其中0~7位代表蓝颜色(0x000000ff）)，8~15位代表绿颜色(0x0000ff00)，16~23位代表红颜色(0x00ff0000)，白色(0x00ffffff) 24~31位是保留位。
	// 这32 bit位值可以组成16 M种不同的颜色，可以表现出真实的色彩
	// 内存管理扩充后 vbe帧缓存区起始线性地址变化 实际物理地址 0xA0000
	int *addr = (int *)0xffff800003000000;
	int i;

	Pos.XResolution = 1440;
	Pos.YResolution = 900;

	Pos.XPosition = 0;
	Pos.YPosition = 0;

	Pos.XCharSize = 8;
	Pos.YCharSize = 16;

	// 内存管理扩充后 vbe帧缓存区起始线性地址变化 实际物理地址 0xA0000 这个根据查询获得 按照实际值设置
	// Pos.FB_addr = (int *)0xffff800000a00000;
	Pos.FB_addr = (int *)0xffff800003000000;
	Pos.FB_length = (Pos.XResolution * Pos.YResolution * 4 + PAGE_4K_SIZE - 1) & PAGE_4K_MASK;

	// 色条展示部分
	// for(i = 0 ;i<1440*20;i++)
	// {
	// 	*((char *)addr+0)=(char)0x00;
	// 	*((char *)addr+1)=(char)0x00;
	// 	*((char *)addr+2)=(char)0xff;
	// 	*((char *)addr+3)=(char)0x00;
	// 	addr +=1;
	// }
	// for(i = 0 ;i<1440*20;i++)
	// {
	// 	*((char *)addr+0)=(char)0x00;
	// 	*((char *)addr+1)=(char)0xff;
	// 	*((char *)addr+2)=(char)0x00;
	// 	*((char *)addr+3)=(char)0x00;
	// 	addr +=1;
	// }
	// for(i = 0 ;i<1440*20;i++)
	// {
	// 	*((char *)addr+0)=(char)0xff;
	// 	*((char *)addr+1)=(char)0x00;
	// 	*((char *)addr+2)=(char)0x00;
	// 	*((char *)addr+3)=(char)0x00;
	// 	addr +=1;
	// }
	// for(i = 0 ;i<1440*20;i++)
	// {
	// 	*((char *)addr+0)=(char)0xff;
	// 	*((char *)addr+1)=(char)0xff;
	// 	*((char *)addr+2)=(char)0xff;
	// 	*((char *)addr+3)=(char)0x00;
	// 	addr +=1;
	// }

	// color_printk(YELLOW,BLACK,"Hello\t\t World!\n");

	load_TR(10);
	set_tss64(0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00,
			  0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00);

	sys_vector_init();
	init_cpu();
	// lds文件中将位置值赋给了这些_xxx 相比较c c中变量会变成符号表 然后去找符号表中的地址 int * a = &test 就是直接吧符号表中test保存的地址给a
	// 对应lds文件中  _xxx 中的值就是对应的地址（不用再间接寻址了） &_xxx 就是他的值
	memory_management_struct.start_code = (unsigned long)&_text;
	memory_management_struct.end_code = (unsigned long)&_etext;
	memory_management_struct.end_data = (unsigned long)&_edata;
	memory_management_struct.end_brk = (unsigned long)&_end;
	color_printk(RED, BLACK, "memory init \n");
	init_memory();

	color_printk(RED, BLACK, "slab init \n");
	slab_init();

	color_printk(RED, BLACK, "frame buffer init \n");
	frame_buffer_init();
	color_printk(WHITE, BLACK, "frame_buffer_init() is OK \n");

	color_printk(RED, BLACK, "pagetable init \n");
	pagetable_init();

	// color_printk(ORANGE, BLACK, "4.memory_management_struct.bits_map:%#018lx\tmemory_management_struct.bits_map+1:%#018lx\tmemory_management_struct.bits_map+2:%#018lx\tzone_struct->page_using_count:%d\tzone_struct->page_free_count:%d\n", *memory_management_struct.bits_map, *(memory_management_struct.bits_map + 1), *(memory_management_struct.bits_map + 2), memory_management_struct.zones_struct->page_using_count, memory_management_struct.zones_struct->page_free_count);
	// color_printk(WHITE, BLACK, "kmalloc test\n");
	// struct Page *page = NULL;
	// void *tmp = NULL;
	// struct Slab *slab = NULL;
	// for (i = 0; i < 16; i++)
	// {
	// 	color_printk(RED, BLACK, "size:%#010x\t", kmalloc_cache_size[i].size);
	// 	color_printk(RED, BLACK, "color_map(before):%#018lx\t", *kmalloc_cache_size[i].cache_pool->color_map);
	// 	tmp = kmalloc(kmalloc_cache_size[i].size, 0);
	// 	if (tmp == NULL)
	// 		color_printk(RED, BLACK, "kmalloc size:%#010x ERROR\n", kmalloc_cache_size[i].size);
	// 	color_printk(RED, BLACK, "color_map(middle):%#018lx\t", *kmalloc_cache_size[i].cache_pool->color_map);
	// 	kfree(tmp);
	// 	color_printk(RED, BLACK, "color_map(after):%#018lx\n", *kmalloc_cache_size[i].cache_pool->color_map);
	// }

	// kmalloc(kmalloc_cache_size[15].size, 0);
	// kmalloc(kmalloc_cache_size[15].size, 0);
	// kmalloc(kmalloc_cache_size[15].size, 0);
	// kmalloc(kmalloc_cache_size[15].size, 0);
	// kmalloc(kmalloc_cache_size[15].size, 0);
	// kmalloc(kmalloc_cache_size[15].size, 0);
	// kmalloc(kmalloc_cache_size[15].size, 0);

	// color_printk(RED, BLACK, "color_map(0):%#018lx,%#018lx\n", kmalloc_cache_size[15].cache_pool->color_map, *kmalloc_cache_size[15].cache_pool->color_map);
	// slab = container_of(list_next(&kmalloc_cache_size[15].cache_pool->list), struct Slab, list);
	// color_printk(RED, BLACK, "color_map(1):%#018lx,%#018lx\n", slab->color_map, *slab->color_map);
	// slab = container_of(list_next(&slab->list), struct Slab, list);
	// color_printk(RED, BLACK, "color_map(2):%#018lx,%#018lx\n", slab->color_map, *slab->color_map);
	// slab = container_of(list_next(&slab->list), struct Slab, list);
	// color_printk(RED, BLACK, "color_map(3):%#018lx,%#018lx\n", slab->color_map, *slab->color_map);

	// i = 1/0;
	// i = *(int *)0xffff80000aa00000;
	// struct Page *page = NULL;
	// color_printk(RED, BLACK, "memory_management_struct.bits_map:%#018lx\n", *memory_management_struct.bits_map);
	// color_printk(RED, BLACK, "memory_management_struct.bits_map:%#018lx\n", *(memory_management_struct.bits_map + 1));

	// page = alloc_pages(ZONE_NORMAL, 64, PG_PTable_Maped | PG_Active | PG_Kernel);

	// for (i = 0; i <= 64; i++)
	// {
	// 	color_printk(INDIGO, BLACK, "page%d\tattribute:%#018lx\taddress:%#018lx\t", i, (page + i)->attribute, (page + i)->PHY_address);
	// 	i++;
	// 	color_printk(INDIGO, BLACK, "page%d\tattribute:%#018lx\taddress:%#018lx\n", i, (page + i)->attribute, (page + i)->PHY_address);
	// }

	// color_printk(RED, BLACK, "memory_management_struct.bits_map:%#018lx\n", *memory_management_struct.bits_map);
	// color_printk(RED, BLACK, "memory_management_struct.bits_map:%#018lx\n", *(memory_management_struct.bits_map + 1));

	// page = alloc_pages(ZONE_NORMAL, 63, 0);
	// page = alloc_pages(ZONE_NORMAL, 63, 0);

	// color_printk(ORANGE, BLACK, "4.memory_management_struct.bits_map:%#018lx\tmemory_management_struct.bits_map+1:%#018lx\tmemory_management_struct.bits_map+2:%#018lx\tzone_struct->page_using_count:%d\tzone_struct->page_free_count:%d\n", *memory_management_struct.bits_map, *(memory_management_struct.bits_map + 1), *(memory_management_struct.bits_map + 2), memory_management_struct.zones_struct->page_using_count, memory_management_struct.zones_struct->page_free_count);

	// for (i = 80; i <= 85; i++)
	// {
	// 	color_printk(INDIGO, BLACK, "page%03d attribute:%#018lx address:%#018lx\t", i, (memory_management_struct.pages_struct + i)->attribute, (memory_management_struct.pages_struct + i)->PHY_address);
	// 	i++;
	// 	color_printk(INDIGO, BLACK, "page%03d attribute:%#018lx address:%#018lx\n", i, (memory_management_struct.pages_struct + i)->attribute, (memory_management_struct.pages_struct + i)->PHY_address);
	// }

	// for (i = 140; i <= 145; i++)
	// {
	// 	color_printk(INDIGO, BLACK, "page%03d attribute:%#018lx address:%#018lx\t", i, (memory_management_struct.pages_struct + i)->attribute, (memory_management_struct.pages_struct + i)->PHY_address);
	// 	i++;
	// 	color_printk(INDIGO, BLACK, "page%03d attribute:%#018lx address:%#018lx\n", i, (memory_management_struct.pages_struct + i)->attribute, (memory_management_struct.pages_struct + i)->PHY_address);
	// }

	// free_pages(page, 1);

	// color_printk(ORANGE, BLACK, "5.memory_management_struct.bits_map:%#018lx\tmemory_management_struct.bits_map+1:%#018lx\tmemory_management_struct.bits_map+2:%#018lx\tzone_struct->page_using_count:%d\tzone_struct->page_free_count:%d\n", *memory_management_struct.bits_map, *(memory_management_struct.bits_map + 1), *(memory_management_struct.bits_map + 2), memory_management_struct.zones_struct->page_using_count, memory_management_struct.zones_struct->page_free_count);

	// for (i = 75; i <= 85; i++)
	// {
	// 	color_printk(INDIGO, BLACK, "page%03d attribute:%#018lx address:%#018lx\t", i, (memory_management_struct.pages_struct + i)->attribute, (memory_management_struct.pages_struct + i)->PHY_address);
	// 	i++;
	// 	color_printk(INDIGO, BLACK, "page%03d attribute:%#018lx address:%#018lx\n", i, (memory_management_struct.pages_struct + i)->attribute, (memory_management_struct.pages_struct + i)->PHY_address);
	// }

	// page = alloc_pages(ZONE_UNMAPED, 63, 0);

	// color_printk(ORANGE, BLACK, "6.memory_management_struct.bits_map:%#018lx\tmemory_management_struct.bits_map+1:%#018lx\tzone_struct->page_using_count:%d\tzone_struct->page_free_count:%d\n", *(memory_management_struct.bits_map + (page->PHY_address >> PAGE_2M_SHIFT >> 6)), *(memory_management_struct.bits_map + 1 + (page->PHY_address >> PAGE_2M_SHIFT >> 6)), (memory_management_struct.zones_struct + ZONE_UNMAPED_INDEX)->page_using_count, (memory_management_struct.zones_struct + ZONE_UNMAPED_INDEX)->page_free_count);

	// free_pages(page, 1);

	// color_printk(ORANGE, BLACK, "7.memory_management_struct.bits_map:%#018lx\tmemory_management_struct.bits_map+1:%#018lx\tzone_struct->page_using_count:%d\tzone_struct->page_free_count:%d\n", *(memory_management_struct.bits_map + (page->PHY_address >> PAGE_2M_SHIFT >> 6)), *(memory_management_struct.bits_map + 1 + (page->PHY_address >> PAGE_2M_SHIFT >> 6)), (memory_management_struct.zones_struct + ZONE_UNMAPED_INDEX)->page_using_count, (memory_management_struct.zones_struct + ZONE_UNMAPED_INDEX)->page_free_count);

	color_printk(RED, BLACK, "interrupt init \n");
#if APIC
	APIC_IOAPIC_init();

	color_printk(RED, BLACK, "keyboard init \n");
	keyboard_init();

	color_printk(RED, BLACK, "mouse init \n");
	mouse_init();

	color_printk(RED, BLACK, "disk init \n");
	disk_init();
#else
	init_8259A();
#endif

	char buf[512];
	color_printk(PURPLE, BLACK, "disk write:\n");
	memset(buf, 0x44, 512);
	IDE_device_operation.transfer(ATA_WRITE_CMD, 0x12345678, 1, (unsigned char *)buf);

	color_printk(PURPLE, BLACK, "disk write end\n");

	color_printk(PURPLE, BLACK, "disk read:\n");
	memset(buf, 0x00, 512);
	IDE_device_operation.transfer(ATA_READ_CMD, 0x12345678, 1, (unsigned char *)buf);

	for (i = 0; i < 512; i++)
		color_printk(BLACK, WHITE, "%02x", buf[i]);
	color_printk(PURPLE, BLACK, "\ndisk read end\n");
	// color_printk(RED, BLACK, "task_init \n");
	// task_init();

#if APIC
	while (1)
	{
		if (p_kb->count)
			analysis_keycode();
		if (p_mouse->count)
			analysis_mousecode();
	}
#else
	while (1)
		;
#endif
}

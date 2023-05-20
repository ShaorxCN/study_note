#include "lib.h"
#include "gate.h"
#include "trap.h"
#include "printk.h"
#include "memory.h"
#include "interrupt.h"
#include "task.h"
#include "cpu.h"
#include "time.h"

#if APIC
#include "APIC.h"
#include "keyboard.h"
#include "mouse.h"
#include "disk.h"
#include "SMP.h"
#else
#include "8259A.h"
#endif

// 段标识符 代码段开始 代码段结束 数据段结束 程序结束 在Kernel.lds中有指明
extern char _text;
extern char _etext;
extern char _edata;
extern char _end;

struct Global_Memory_Descriptor memory_management_struct = {{0}, 0};
int global_i = 0;
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
	spin_init(&Pos.printk_lock);

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
	spin_init(&Pos.printk_lock);

	load_TR(10);
	set_tss64(TSS64_Table, _stack_start, _stack_start, _stack_start,
			  0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00);

	sys_vector_init();
	// init_cpu();
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

	color_printk(RED, BLACK, "interrupt init \n");
#if APIC
	unsigned int *tss = NULL;
	APIC_IOAPIC_init();

	color_printk(RED, BLACK, "keyboard init \n");
	keyboard_init();

	color_printk(RED, BLACK, "mouse init \n");
	mouse_init();

	color_printk(RED, BLACK, "disk init \n");
	disk_init();

	Local_APIC_init();
	color_printk(RED, BLACK, "ICR init\n");

	// *(unsigned char *)0xffff800000020000 = 0xf4; // hlt

	// // msr配置icr 先发的INIT 再发start-up
	// __asm__ __volatile__("movq $0x00,%%rdx \n\t"
	// 					 "movq $0xc4500,%%rax \n\t"
	// 					 "movq $0x830,%%rcx \n\t" // INIT IPI
	// 					 "wrmsr \n\t"
	// 					 "movq $0x00,%% rdx \n\t "
	// 					 "movq $0xc4620,%%rax \n\t" // 物理模式 start up  向所有发送(除自己) 向量号0x20 那么startup的其实地址应该是000vv000 就是20000
	// 					 "movq $0x830,%%rcx \n\t"	// start-up IPI
	// 					 "wrmsr \n\t"
	// 					 "movq $0x00,%% rdx \n\t "
	// 					 "movq $0xc4620,%%rax \n\t"
	// 					 "movq $0x830,%%rcx \n\t" // start-up IPI again
	// 					 "wrmsr \n\t" ::
	// 						 : "memory");
	SMP_init();

	struct INT_CMD_REG icr_entry;
	icr_entry.vector = 0x00;
	icr_entry.deliver_mode = APIC_ICR_IOAPIC_INIT;
	icr_entry.dest_mode = ICR_IOAPIC_DELV_PHYSICAL;
	icr_entry.deliver_status = APIC_ICR_IOAPIC_Idle;
	icr_entry.res_1 = 0;
	icr_entry.level = ICR_LEVEL_DE_ASSERT;
	icr_entry.trigger = APIC_ICR_IOAPIC_Edge;
	icr_entry.res_2 = 0;
	icr_entry.dest_shorthand = ICR_ALL_EXCLUDE_Self;
	icr_entry.res_3 = 0;
	icr_entry.destination.x2apic_destination = 0x00;

	// wrmsr(0x830, 0xc4500); // INIT IPI

	// wrmsr(0x830, 0xc4620); // Start-up IPI
	// wrmsr(0x830, 0xc4620); // Start-up IPI
	wrmsr(0x830, *(unsigned long *)&icr_entry); // INIT IPI
	// 这边双核四线程
	for (global_i = 1; global_i < 4; global_i++)
	{
		spin_lock(&SMP_lock);

		_stack_start = (unsigned long)kmalloc(STACK_SIZE, 0) + STACK_SIZE;
		tss = (unsigned int *)kmalloc(128, 0);
		set_tss_descriptor(10 + global_i * 2, tss);
		set_tss64(tss, _stack_start, _stack_start, _stack_start, _stack_start, _stack_start, _stack_start, _stack_start, _stack_start, _stack_start, _stack_start);
		icr_entry.vector = 0x20;
		icr_entry.deliver_mode = ICR_Start_up;
		icr_entry.dest_shorthand = ICR_No_Shorthand;
		icr_entry.destination.x2apic_destination = global_i;
		wrmsr(0x830, *(unsigned long *)&icr_entry); // Start-up IPI
		wrmsr(0x830, *(unsigned long *)&icr_entry); // Start-up IPI
	}

	// // ipi 中断测试 通过icr发送
	// icr_entry.vector = 0xc8;
	// icr_entry.destination.x2apic_destination = 1;
	// icr_entry.deliver_mode = APIC_ICR_IOAPIC_Fixed;
	// wrmsr(0x830, *(unsigned long *)&icr_entry);
	// icr_entry.vector = 0xc9;
	// wrmsr(0x830, *(unsigned long *)&icr_entry);

#else
	init_8259A();
#endif
	struct time time;
	get_cmos_time(&time);
	color_printk(RED, BLACK, "year:%#010x,month:%#010x,day:%#010x,hour:%#010x,mintue:%#010x,second:%#010x\n", time.year, time.month, time.day, time.hour, time.minute, time.second);

	// char buf[512];
	// color_printk(PURPLE, BLACK, "disk write:\n");
	// memset(buf, 0x44, 512);
	// IDE_device_operation.transfer(ATA_WRITE_CMD, 0x12, 1, (unsigned char *)buf);

	// color_printk(PURPLE, BLACK, "disk write end\n");

	// color_printk(PURPLE, BLACK, "disk read:\n");
	// memset(buf, 0x00, 512);
	// IDE_device_operation.transfer(ATA_READ_CMD, 0x12, 1, (unsigned char *)buf);

	// for (i = 0; i < 512; i++)
	// 	color_printk(BLACK, WHITE, "%02x", buf[i]);
	// color_printk(PURPLE, BLACK, "\ndisk read end\n");
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

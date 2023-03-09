#include "lib.h"
#include "gate.h"
#include "trap.h"
#include "printk.h"
#include "memory.h"

// 段标识符 代码段开始 代码段结束 数据段结束 程序结束 在Kernel.lds中有指明
extern char _text;
extern char _etext;
extern char _edata;
extern char _end;

struct Global_Memory_Descriptor memory_management_strcut = {{0}, 0};

void Start_Kernel(void)
{
	// header.S 中将帧缓存的物理地址(0xe0000000) 映射到0xffff800000a00000和0xa00000处
	// loader设置了显示模式(（模式号：0x180、分辨率：1440×900、颜色深度：32 bit）)
	// Loader引导加载程序设置的显示模式可支持32位颜色深度的像素点，其中0~7位代表蓝颜色(0x000000ff）)，8~15位代表绿颜色(0x0000ff00)，16~23位代表红颜色(0x00ff0000)，白色(0x00ffffff) 24~31位是保留位。
	// 这32 bit位值可以组成16 M种不同的颜色，可以表现出真实的色彩
	int *addr = (int *)0xffff800000a00000;
	int i;

	Pos.XResolution = 1440;
	Pos.YResolution = 900;

	Pos.XPosition = 0;
	Pos.YPosition = 0;

	Pos.XCharSize = 8;
	Pos.YCharSize = 16;

	Pos.FB_addr = (int *)0xffff800000a00000;
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

	load_TR(8);
	set_tss64(0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00,
			  0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00);

	sys_vector_init();

	// lds文件中将位置值赋给了这些_xxx 相比较c c中变量会变成符号表 然后去找符号表中的地址 int * a = &test 就是直接吧符号表中test保存的地址给a
	// 对应lds文件中  _xxx 中的值就是对应的地址（不用再间接寻址了） &_xxx 就是他的值
	memory_management_strcut.start_code = (unsigned long)&_text;
	memory_management_strcut.end_code = (unsigned long)&_etext;
	memory_management_strcut.end_data = (unsigned long)&_edata;
	memory_management_strcut.end_brk = (unsigned long)&_end;
	color_printk(RED, BLACK, "memory init \n");
	init_memory();

	// i = 1/0;
	// i = *(int *)0xffff80000aa00000;

	while (1)
		;
}

#include "cpu.h"
#include "printk.h"

/*

*/

void init_cpu(void)
{
    int i, j;
    unsigned int CpuFacName[4] = {0, 0, 0, 0};
    char FactoryName[17] = {0};

    // 功能号 0,0 获取厂商标识字符 返回的12个ascii字符依次存放在ebx edx ecx所以是CpuFacName的132index
    // eax返回处理器的最大基础功能号
    get_cpuid(0, 0, &CpuFacName[0], &CpuFacName[1], &CpuFacName[2], &CpuFacName[3]);

    *(unsigned int *)&FactoryName[0] = CpuFacName[1];

    *(unsigned int *)&FactoryName[4] = CpuFacName[3];

    *(unsigned int *)&FactoryName[8] = CpuFacName[2];

    // 字符串结束符
    FactoryName[12] = '\0';
    color_printk(YELLOW, BLACK, "%s\t%#010x\t%#010x\t%#010x\n", FactoryName, CpuFacName[1], CpuFacName[3], CpuFacName[2]);

    // EAX=80000002h、80000003h、80000004h：返回处理器名称/商标字符串 80000005h 备用
    // 每次调用CPUID分别在EAX、EBX、ECX、EDX中返回16个ASCII字符，处理器名称/商标字串最多48个字符，前导字符为空格，结束字符为 NULL，
    for (i = 0x80000002; i < 0x80000005; i++)
    {
        get_cpuid(i, 0, &CpuFacName[0], &CpuFacName[1], &CpuFacName[2], &CpuFacName[3]);

        *(unsigned int *)&FactoryName[0] = CpuFacName[0];

        *(unsigned int *)&FactoryName[4] = CpuFacName[1];

        *(unsigned int *)&FactoryName[8] = CpuFacName[2];

        *(unsigned int *)&FactoryName[12] = CpuFacName[3];

        FactoryName[16] = '\0';
        color_printk(YELLOW, BLACK, "%s", FactoryName);
    }
    color_printk(YELLOW, BLACK, "\n");

    // Version Informatin Type,Family,Model,and Stepping ID
    get_cpuid(1, 0, &CpuFacName[0], &CpuFacName[1], &CpuFacName[2], &CpuFacName[3]);
    color_printk(YELLOW, BLACK, "Family Code:%#010x,Extended Family:%#010x,Model Number:%#010x,Extended Model:%#010x,Processor Type:%#010x,Stepping ID:%#010x\n", (CpuFacName[0] >> 8 & 0xf), (CpuFacName[0] >> 20 & 0xff), (CpuFacName[0] >> 4 & 0xf), (CpuFacName[0] >> 16 & 0xf), (CpuFacName[0] >> 12 & 0x3), (CpuFacName[0] & 0xf));

    // get Linear/Physical Address size
    get_cpuid(0x80000008, 0, &CpuFacName[0], &CpuFacName[1], &CpuFacName[2], &CpuFacName[3]);
    color_printk(YELLOW, BLACK, "Physical Address size:%08d,Linear Address size:%08d\n", (CpuFacName[0] & 0xff), (CpuFacName[0] >> 8 & 0xff));

    // max cpuid operation code
    get_cpuid(0, 0, &CpuFacName[0], &CpuFacName[1], &CpuFacName[2], &CpuFacName[3]);
    color_printk(WHITE, BLACK, "MAX Basic Operation Code :%#010x\t", CpuFacName[0]);

    get_cpuid(0x80000000, 0, &CpuFacName[0], &CpuFacName[1], &CpuFacName[2], &CpuFacName[3]);
    color_printk(WHITE, BLACK, "MAX Extended Operation Code :%#010x\n", CpuFacName[0]);
}
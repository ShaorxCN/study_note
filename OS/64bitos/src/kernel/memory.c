#include "lib.h"
#include "memory.h"


// 获取内存信息
void init_memory()
{
    int i,j;
    unsigned long TotalMem = 0;
    struct E820 *p = NULL;

    color_printk(BLUE,BLACK,"Display Physics Address MAP,Type(1:RAM,2:ROM or Reserved,3:ACPI Reclaim Memory,4:ACPI NVS Memory,Others:Undefine)\n");
    p=(struct E820 *)0xffff800000007e00;

    for(i=0;i<32;i++)
    {
        color_printk(ORANGE,BLACK,"Address:%#018lx\tLength:%#018lx\tType:%#010x\n",p->address,p->length,p->type);
        
        if(p->type ==1)
            TotalMem+=p->length;
        memory_management_strcut.e820[i].address += p->address;
        memory_management_strcut.e820[i].length += p->length;
        memory_management_strcut.e820[i].type = p->type;
        memory_management_strcut.e820_length = i;

        p++;
        if(p->type > 4)
            break;
    }

    // tip 这里相当于带前缀的long类型16进制 宽度18 前补0
    color_printk(ORANGE,BLACK,"OS Can Used Total RAM:%#018lx\n",TotalMem);

    // 获取2m分页下的可通内存
    TotalMem = 0;
    for(i=0;i<memory_management_strcut.e820_length;i++)
    {
        unsigned long start,end;
        if(memory_management_strcut.e820[i].type!=1)
            continue;
        // 向上对齐的开始地址
        start = PAGE_2M_ALIGN(memory_management_strcut.e820[i].address);
        // 对齐的结束地址 看是否够2M 多的话也截断
        end = ((memory_management_strcut.e820[i].address+memory_management_strcut.e820[i].length) >> PAGE_2M_SHIFT)<<PAGE_2M_SHIFT;
        // 不够就算了
        if(end <= start)
            continue;
        TotalMem +=(end-start)>>PAGE_2M_SHIFT;
    }
    color_printk(ORANGE,BLACK,"OS Can Used Total 2M PAGEs:%#010x=%010d\n",TotalMem,TotalMem);
}
#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "printk.h"
#include "lib.h"


//	8Bytes per cell
#define PTRS_PER_PAGE	512

// 初始线性地址
#define PAGE_OFFSET	((unsigned long)0xffff800000000000)


// 大中小分页的size 容量
#define PAGE_1G_SHIFT	30
#define PAGE_2M_SHIFT	21
#define PAGE_4K_SHIFT	12

#define PAGE_2M_SIZE	(1UL << PAGE_2M_SHIFT)
#define PAGE_4K_SIZE	(1UL << PAGE_4K_SHIFT)

// 低位掩码
#define PAGE_2M_MASK	(~ (PAGE_2M_SIZE - 1))
#define PAGE_4K_MASK	(~ (PAGE_4K_SIZE - 1))

// 对齐宏 向上对齐
#define PAGE_2M_ALIGN(addr) (((unsigned long)(addr) + PAGE_2M_SIZE - 1) & PAGE_2M_MASK)
#define PAGE_4K_ALIGN(addr) (((unsigned long)(addr) + PAGE_4K_SIZE - 1) & PAGE_4K_MASK)

// 前10m空间可用的 虚拟地址和物理地址的转换 这里看header.S 最后PDE 0-4 entry
#define Virt_To_Phy(addr)	((unsigned long)(addr) - PAGE_OFFSET)
#define Phy_To_Virt(addr)	((unsigned long *)((unsigned long)(addr) + PAGE_OFFSET))



//  定义不优化的内存结构/不对齐 紧凑模式
struct E820
{
    unsigned long address;
    unsigned long length;
    unsigned int type;
}__attribute__((__packed__));


struct Global_Memory_Descriptor
{
    struct E820 e820[32];
    unsigned long e820_length;
};

extern struct Global_Memory_Descriptor memory_management_strcut;


#endif
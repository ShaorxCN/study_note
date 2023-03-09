#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "printk.h"
#include "lib.h"

//	8Bytes per cell
#define PTRS_PER_PAGE 512

// 初始线性地址
#define PAGE_OFFSET ((unsigned long)0xffff800000000000)

// 大中小分页的size 容量
#define PAGE_1G_SHIFT 30
#define PAGE_2M_SHIFT 21
#define PAGE_4K_SHIFT 12

#define PAGE_2M_SIZE (1UL << PAGE_2M_SHIFT)
#define PAGE_4K_SIZE (1UL << PAGE_4K_SHIFT)

// 低位掩码
#define PAGE_2M_MASK (~(PAGE_2M_SIZE - 1))
#define PAGE_4K_MASK (~(PAGE_4K_SIZE - 1))

// 对齐宏 向上对齐
#define PAGE_2M_ALIGN(addr) (((unsigned long)(addr) + PAGE_2M_SIZE - 1) & PAGE_2M_MASK)
#define PAGE_4K_ALIGN(addr) (((unsigned long)(addr) + PAGE_4K_SIZE - 1) & PAGE_4K_MASK)

// 前10m空间可用的 虚拟地址和物理地址的转换 这里看header.S 最后PDE 0-4 entry
#define Virt_To_Phy(addr) ((unsigned long)(addr)-PAGE_OFFSET)
#define Phy_To_Virt(addr) ((unsigned long *)((unsigned long)(addr) + PAGE_OFFSET))

//  定义不优化的内存结构/不对齐 紧凑模式
struct E820
{
    unsigned long address;
    unsigned long length;
    unsigned int type;
} __attribute__((__packed__));

struct Global_Memory_Descriptor
{
    struct E820 e820[32];      // e820 内存段数组
    unsigned long e820_length; // 数组长度

    unsigned long *bits_map;   // 物理地址空间页映射位图
    unsigned long bits_size;   // 物理地址空间页数量 包含空洞 rom等 直接用总内存算的
    unsigned long bits_length; // 位图长度 (8字节对齐后的 大于等于)

    struct Page *pages_struct;  // 页数组指针
    unsigned long page_size;    // 结构体总数
    unsigned long pages_length; // 数组长度

    struct Zone *zones_struct;  // zone数组指针
    unsigned long zones_size;   // zone结构体数量
    unsigned long zones_length; // 数组长度

    unsigned long start_code, end_code, end_data, end_brk; // 内核程序的起始代码段地址/结束地址/结束数据段地址/内核程序的结束地址

    unsigned long end_of_struct; // 内存页管理结构的结尾地址
};

struct Page
{
    struct Zone *zone_struct;      // 指向本页所属的区域
    unsigned long PHY_address;     // 物理地址
    unsigned long attribute;       // 属性
    unsigned long reference_count; // 该页的引用次数
    unsigned long age;             // 该页的创建时间
};

struct Zone
{
    struct Page *pages_group;         // 页数组指针
    unsigned long pages_length;       // 页数量
    unsigned long zone_start_address; // 对齐后区域开始地址
    unsigned long zone_end_address;   // 对齐后区域结束地址
    unsigned long zone_length;        // 对齐后内存长度
    unsigned long attribute;          // 属性

    struct Global_Memory_Descriptor *GMD_struct; // 全局描述符指针

    unsigned long page_using_count; // 已使用内存页数
    unsigned long page_free_count;  // 空闲页数
    unsigned long total_pages_link; // 区域内页被引用次数  可能一个物理页被映射到多个虚拟地址/线性地址
};

extern struct Global_Memory_Descriptor memory_management_strcut;

#endif
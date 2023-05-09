#include "SMP.h"
#include "lib.h"
#include "printk.h"
#include "cpu.h"
void SMP_init()
{
    int i;
    unsigned int a, b, c, d;
    for (i = 0;; i++)
    {
        get_cpuid(0xb, i, &a, &b, &c, &d);
        if ((c >> 8 & 0xff) == 0)
            break;
        color_printk(WHITE, BLACK, "local APIC ID Package_../Core_2/SMT_1,type(%x) Width:%#010x,num of logical processor(%x)\n", c >> 8 & 0xff, a & 0x1f, b & 0xff);
    }
    color_printk(WHITE, BLACK, "x2APIC ID level:%#010x\tx2APIC ID the current logical processor:%#010x\n", c & 0xff, d);

    // 复制ap引导程序到0x20000处
    color_printk(WHITE, BLACK, "SMP copy byte:%#010x\n", (unsigned long)&_APU_boot_end - (unsigned long)&_APU_boot_start);
    memcpy(_APU_boot_start, (unsigned char *)0xffff800000020000, (unsigned long)&_APU_boot_end - (unsigned long)&_APU_boot_start);
}
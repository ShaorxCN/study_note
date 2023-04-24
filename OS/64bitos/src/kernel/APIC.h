#ifndef __APIC_H__
#define __APIC_H__

#include "linkage.h"
#include "ptrace.h"
#include "interrupt.h"

// 协助间接访问寄存器
struct IOAPIC_map
{
    unsigned int physical_address;
    unsigned char *virtual_index_address;
    unsigned int *virtual_data_address;
    unsigned int *virtual_EOI_address;
} ioapic_map;

unsigned long ioapic_rte_read(unsigned char index);
void ioapic_rte_write(unsigned char index, unsigned long value);
void IOAPIC_pagetable_remap();
void do_IRQ(struct pt_regs *regs, unsigned long nr);

void APIC_IOAPIC_init();
void Local_APIC_init();
void IOAPIC_init();
#endif

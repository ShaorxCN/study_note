#ifndef __SMP_H__

#define __SMP_H__
#include "spinlock.h"
extern unsigned char _APU_boot_start[];
extern unsigned char _APU_boot_end[];
extern int global_i;
#define SMP_cpu_id() (current->cpu_id)

spinlock_T SMP_lock;
void SMP_init();
void Start_SMP();

#endif

#ifndef __SMP_H__

#define __SMP_H__
#include "spinlock.h"
extern unsigned char _APU_boot_start[];
extern unsigned char _APU_boot_end[];

spinlock_T SMP_lock;
void SMP_init();
void Start_SMP();

#endif

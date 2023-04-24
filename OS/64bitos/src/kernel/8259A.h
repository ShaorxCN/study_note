#ifndef __8259A_H__
#define __8259A_H__

#include "linkage.h"
#include "ptrace.h"

void init_8259A();

void do_IRQ(struct pt_regs *regs, unsigned long nr);

#endif

#include "linkage.h"

.balign 0x1000  // .balign x, y 从当前位置开始 填充y 知道pc地址位x的倍数为止 这里就是4k对齐的意思

.text
.code16

ENTRY(_APU_boot_start)

_APU_boot_base= . // 纪录当前pc值

    cli

    wbinvd  // 讲处理器缓存同步到内存中

    mov %cs,%ax             // ax=2000
    mov %ax,%ds
    mov %ax,%es
    mov %ax,%ss
    mov %ax,%fs
    mov %ax,%gs

    movl $(_APU_boot_tmp_Stack_end-_APU_boot_base),%esp  // 偏移值  基于 ss=0x2000 实模式下就是20000

    // start ipi 无法到1m以上地址 所以代码复制到0x20000处 这样导致部分链接后的绝对地址不正确 所以需要重新计算
	mov	%cs,	%ax
	movzx %ax,	%esi
	shll $4,	%esi           // esi左移4位 cs是2000 20000?  实际物理地址

#	set gdt and 32&64 code address

	leal	(_APU_Code32 - _APU_boot_base)(%esi),	%eax        // code 和base间的偏移 相当于code32基于esi的绝对地址？
	movl	%eax,	_APU_Code32_vector - _APU_boot_base         //  这个值放到32vector?

	leal	(_APU_Code64 - _APU_boot_base)(%esi),	%eax
	movl	%eax,	_APU_Code64_vector - _APU_boot_base         // code64的物理地址放到64vector

	leal	(_APU_tmp_GDT - _APU_boot_base)(%esi),	%eax       // 同理 apu_tmp_GDT的物理地址到.long那个位置
	movl	%eax,	(_APU_tmp_GDT + 2 - _APU_boot_base)
	
#	load idt gdt                                    相对地址加载idtr gdtr
	
	lidtl	_APU_tmp_IDT - _APU_boot_base
	lgdtl	_APU_tmp_GDT - _APU_boot_base

#	enable protected mode  开启保护模式

	smsw	%ax
	bts	$0	,%ax
	lmsw	%ax

#	go to 32 code
	ljmpl	*(_APU_Code32_vector - _APU_boot_base)  // 远跳转 实际保存的是code32的物理地址 就在保护模式中执行code32代码 0x08 : offset

.code32
.balign 4
_APU_Code32:
#	go to 64 code
	
	mov	$0x10,	%ax
	mov	%ax,	%ds
	mov	%ax,	%es
	mov	%ax,	%ss
	mov	%ax,	%fs
	mov	%ax,	%gs

	leal	(_APU_boot_tmp_Stack_end - _APU_boot_base)(%esi),	%eax         // 计算实际esp
	movl	%eax,	%esp
	
#	open PAE

	movl	%cr4,	%eax
	bts	$5,	%eax
	movl	%eax,	%cr4

#	set page table

	movl	$0x90000,	%eax  // 使用loader.asm中的临时页表
	movl	%eax,	%cr3

#	enable long mode

	movl	$0xC0000080,	%ecx
	rdmsr

	bts	$8,	%eax
	wrmsr

#	enable PE & paging

	movl	%cr0,	%eax
	bts	$0,	%eax
	bts	$31,	%eax
	movl	%eax,	%cr0
	
	ljmp	*(_APU_Code64_vector - _APU_boot_base)(%esi)  // 切换到ia32-e

.code64
.balign 4
_APU_Code64:
#	go to head.S
	movq	$0x20,	%rax
	movq	%rax,	%ds
	movq	%rax,	%es
	movq	%rax,	%fs
	movq	%rax,	%gs
	movq	%rax,	%ss

	hlt

.balign 4
_APU_tmp_IDT:
	.word	0
	.word	0,0

.balign 4
_APU_tmp_GDT:
	.short	_APU_tmp_GDT_end - _APU_tmp_GDT - 1
	.long	_APU_tmp_GDT - _APU_boot_base
	.short	0
	.quad	0x00cf9a000000ffff
	.quad	0x00cf92000000ffff
	.quad	0x0020980000000000
	.quad	0x0000920000000000
_APU_tmp_GDT_end:

.balign 4
_APU_Code32_vector:
	.long	_APU_Code32 - _APU_boot_base
	.word	0x08,0	

.balign 4
_APU_Code64_vector:
	.long	_APU_Code64 - _APU_boot_base
	.word	0x18,0	

.balign 4
_APU_boot_tmp_Stack_start:
	.org	0x400            // 保证了1kb空间 开始的部分是程序 高位剩下的是做栈 下面的stack end就是栈顶
_APU_boot_tmp_Stack_end:
ENTRY(_APU_boot_end)
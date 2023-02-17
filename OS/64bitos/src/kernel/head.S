.section .text
// 在GAS编译器中，使用标识符_start作为程序的默认起始位置，同时还要使用伪指令.globl对_start标识符加以修饰。
//如果不使用.globl修饰_start标识符的话，链接过程会出现警告ld: warning: cannot find entry symbol _start; defaulting to ffff800000100000。
.globl _start

_start:

	mov	$0x10,	%ax
	mov	%ax,	%ds
	mov	%ax,	%es
	mov	%ax,	%fs
	mov	%ax,	%ss
	mov	$0x7E00,	%esp

//=======	load GDTR

	lgdt	GDT_POINTER(%rip)

//=======	load	IDTR

	lidt	IDT_POINTER(%rip)

	mov	$0x10,	%ax
	mov	%ax,	%ds
	mov	%ax,	%es
	mov	%ax,	%fs
	mov	%ax,	%gs
	mov	%ax,	%ss

	movq	$0x7E00,	%rsp

//=======	load	cr3

	movq	$0x101000,	%rax
	movq	%rax,		%cr3
	movq	switch_seg(%rip),	%rax
	pushq	$0x08
	pushq	%rax
	lretq

//=======	64-bit mode code

switch_seg:
	.quad	entry64

entry64:
	movq	$0x10,	%rax
	movq	%rax,	%ds
	movq	%rax,	%es
	movq	%rax,	%gs
	movq	%rax,	%ss
	movq	$0xffff800000007E00,	%rsp		/* rsp address */

	movq	go_to_kernel(%rip),	%rax		/* movq address */
	pushq	$0x08
	pushq	%rax
	lretq

go_to_kernel:
	.quad	Start_Kernel
//=======	init page  初始化页表
.align 8                               // 8字节对齐
// 确定该结构存放在段的相对偏移0x1000处（相对上面的.text）  .org new-lc , fill 如果作用填充当前位置到新位置 不能跨段
// 这里header程序会通过连接器讲其实线性地址设置在0xffff800000000000+0x100000 处 那么页表位置就是
// 0xffff800000100000+0x1000=0xffff00000101000
.org	0x1000                

//  四级映射表项
// PML4E的35-12位为PDPT页目录指针表物理地址的高24位，低12位置0。 这里[0]一个0代表一个4bit的0
// 这里讲线性地址0和0xffff800000000000 映射位同一物理地址 因为0对应的0x102007 后面index都是0
// 0xffff800000000000 ffff符号拓展位无视后面9位 1000 0000 0 是256index 也是0x102007 这里10MB= 5*512*4kb 对应5个PDE项
__PML4E:

	.quad	0x102007    // [00000000]00102007  0000 0000 0001 0000 0010 0000 0000 0111   bit[35-12] 0x000102  低12位那就是3个16进制0那么地址就是 0x102000
	.fill	255,8,0
	.quad	0x102007
	.fill	255,8,0

.org	0x2000           // 10200开始

__PDPTE:
	
	.quad	0x103003   //  [00000000]00103003 0000 0000 0001 0000 0011 0000 0000 0011 同理 bit[35-12]0x000103 低12bit置零 =》0x103000  
	.fill	511,8,0

.org	0x3000

// page directory entry 页表目录项
__PDE:

	.quad	0x000083	
	.quad	0x200083
	.quad	0x400083
	.quad	0x600083
	.quad	0x800083
	.quad	0xe0000083		/*0x a00000*/
	.quad	0xe0200083
	.quad	0xe0400083
	.quad	0xe0600083		/*0x1000000*/
	.quad	0xe0800083
	.quad	0xe0a00083
	.quad	0xe0c00083
	.quad	0xe0e00083
	.fill	499,8,0

//============================== 初始化GDT IDT 以及TSS
//=======    GDT_Table 

.section .data                  // 定义数据段 可读写 一下代码都属于数据节

.globl GDT_Table                // 定义全局的GDT表 .global symbol 代表symbol在obj中全局可见 这里仅仅是声明 需要symbol的定义 不然报错

GDT_Table:
    .quad    0x0000000000000000    /*0 NULL descriptor 00*/
    .quad    0x0020980000000000    /*1 KERNEL Code 64-bit Segment 08*/
    .quad    0x0000920000000000    /*2 KERNEL Data 64-bit Segment 10*/
    .quad    0x0020f80000000000    /*3 USER    Code 64-bit Segment 18*/
    .quad    0x0000f20000000000    /*4 USER    Data 64-bit Segment 20*/
    .quad    0x00cf9a000000ffff    /*5 KERNEL Code 32-bit Segment 28*/
    .quad    0x00cf92000000ffff    /*6 KERNEL Data 32-bit Segment 30*/
    .fill    10,8,0                /*8 ~ 9    TSS (jmp one segment <7>) in long-mode 128-bit 40*/
GDT_END:

GDT_POINTER:                                            // 区分 这里是初始化结构以及定位指针 不是GDTR用 所以非48bit
GDT_LIMIT:    .word    GDT_END - GDT_Table - 1         //  .word 等价于 .int .long  4byte 32bit len 文档里说是根据目标机器 The size of the number emitted, and its byte order, depend on what target computer the assembly is for.
GDT_BASE:     .quad    GDT_Table                       //  64bit base

//=======     IDT_Table

.globl IDT_Table
// .fill repeat , size（默认1） , value（默认0） size最多8 （byte） 如果size大于value 高位填充0
// 这里解释下  32bit下 结构为256*8byte  但是64bit下  256*16byte
IDT_Table:
    .fill 512,8,0                       
IDT_END:

IDT_POINTER:
IDT_LIMIT:    .word    IDT_END - IDT_Table - 1
IDT_BASE:     .quad    IDT_Table

//=======     TSS64_Table

.globl        TSS64_Table

TSS64_Table:
    .fill 13,8,0
TSS64_END:

TSS64_POINTER:
TSS64_LIMIT:    .word    TSS64_END - TSS64_Table - 1
TSS64_BASE:     .quad    TSS64_T
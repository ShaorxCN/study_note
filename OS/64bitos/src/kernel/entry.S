#include "linkage.h"

// 设置在处理异常时 为了保存现场 各个寄存器在栈中的保存顺序
R15 =    0x00
R14 =    0x08
R13 =    0x10
R12 =    0x18
R11 =    0x20
R10 =    0x28
R9  =    0x30
R8  =    0x38
RBX =    0x40
RCX =    0x48
RDX =    0x50
RSI =    0x58
RDI =    0x60
RBP =    0x68
DS  =    0x70
ES  =    0x78
RAX =    0x80
FUNC    = 0x88
ERRCODE = 0x90
RIP =    0x98
CS  =    0xa0
RFLAGS =    0xa8
OLDRSP =    0xb0
OLDSS  =    0xb8

// 恢复中断
RESTORE_ALL:
	popq	%r15;		
	popq	%r14;		
	popq	%r13;		
	popq	%r12;		
	popq	%r11;		
	popq	%r10;		
	popq	%r9;		
	popq	%r8;		
	popq	%rbx;		
	popq	%rcx;		
	popq	%rdx;		
	popq	%rsi;		
	popq	%rdi;		
	popq	%rbp;		
	popq	%rax;		
	movq	%rax,	%ds;	        // 因为64汇编中 push cs ds es ss pop ds/es/ss 是无效的 所以rax代替 
	popq	%rax;		
	movq	%rax,	%es;	
	popq	%rax;		
	addq	$0x10,	%rsp;	       // 增加16B  为了弹出栈中的变量FUNC 和ERRCODE 这里保存着实际是中断处理程序的地址 所以要先弹出 方便iretq还原现场
	iretq;


ret_from_exception:
	/*GET_CURRENT(%ebx)	need rewrite*/
ENTRY(ret_from_intr)
	jmp	RESTORE_ALL	/*need rewrite*/


ENTRY(divide_error)
	pushq	$0                           // 压入0 占位ERRCODE 因为#DE没有ERRCODE
	pushq	%rax
	leaq	do_divide_error(%rip),	%rax
	xchgq	%rax,	(%rsp)               // 交换rax 和rsp内存处的值   就是push do_divide_error到栈中

// 各自errcode 和异常处理程序地址保存后的通用步骤
error_code:
	pushq	%rax
	movq	%es,	%rax
	pushq	%rax
	movq	%ds,	%rax
	pushq	%rax
	xorq	%rax,	%rax

	pushq	%rbp
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbx
	pushq	%r8
	pushq	%r9
	pushq	%r10
	pushq	%r11
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15	
	
	cld                         // si->+
	movq	ERRCODE(%rsp),	%rsi   // 定位错误码栈中的位置 存储到rsi  传递参数寄存器的顺位2
	movq	FUNC(%rsp),	%rdx	   // 定位函数 存储到rdx                  

	movq	$0x10,	%rdi           // 因为可能存在特权级切换 所以将ds es加载内核层数据段
	movq	%rdi,	%ds
	movq	%rdi,	%es

	movq	%rsp,	%rdi          // 栈指针保存在rdi 传递参数寄存器的顺位1
	////GET_CURRENT(%ebx)

	callq 	*%rdx                 // 调用异常处理函数 *表示跳转的参数是绝对地址 否则是相对地址

	jmp	ret_from_exception	



// 不可屏蔽中断  是一个中断 也没有错误码
ENTRY(nmi)
	pushq	%rax            // 等价errcode 入栈
	cld;			
	pushq	%rax;	        // 等价FUNC入栈
	
	pushq	%rax
	movq	%es,	%rax
	pushq	%rax
	movq	%ds,	%rax
	pushq	%rax
	xorq	%rax,	%rax   // 清零
	
	pushq	%rbp;		
	pushq	%rdi;		
	pushq	%rsi;		
	pushq	%rdx;		
	pushq	%rcx;		
	pushq	%rbx;		
	pushq	%r8;		
	pushq	%r9;		
	pushq	%r10;		
	pushq	%r11;		
	pushq	%r12;		
	pushq	%r13;		
	pushq	%r14;		
	pushq	%r15;
	
	movq	$0x10,	%rdx;	
	movq	%rdx,	%ds;	
	movq	%rdx,	%es;
	
	movq	$0,	%rsi
	movq	%rsp,	%rdi

	callq	do_nmi

	jmp	RESTORE_ALL


ENTRY(invalid_TSS)
    push %rax                             // 错误码
    leaq do_invalid_TSS(%rip),%rax        // 处理函数绝对地址
    xchgq %rax,(%rsp)
    jmp error_code


ENTRY(page_fault)
	pushq	%rax
	leaq	do_page_fault(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code



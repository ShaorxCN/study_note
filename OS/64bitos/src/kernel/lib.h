#ifndef __LIB_H__
#define __LIB_H__


#define NULL 0
/*
	计算字符串长度
    cld df=0 控制内存地址向高位递增
    repne 当ecx!=0且zf=0时 重复执行后面的指令 执行一次ecx-1
    notl 表示not的l（long 32bit 4B版本）
    D 表示使用RDI EDI DI寄存器 "0"表示和%0关联作为输入 这样取反等操作针对=c指定的ecx再输出到__res
    scasb scan string byte 比较al 和 byte of [ES:EDI] 如果df=0 inc edi
    执行一次ecx-1 初始时0xffffffff 所以取反再减一就是长度 notl和decl(例如两次 变成fffffffd 取反00000002 减去1（'0'本身）就是1)
*/
static inline int strlen(char * String)
{
    // register 关键字说明后面的变量访问频率较高 告诉编译器尽量将他存储在寄存器中
	register int __res;
	__asm__	__volatile__	(	"cld	\n\t"
					"repne	\n\t"
					"scasb	\n\t"
					"notl	%0	\n\t"
					"decl	%0	\n\t"
					:"=c"(__res)
					:"D"(String),"a"(0),"0"(0xffffffff)
					:
				);
	return __res;
}
#endif

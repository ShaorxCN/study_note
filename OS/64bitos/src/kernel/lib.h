#ifndef __LIB_H__
#define __LIB_H__

#define NULL 0

// 根据结构体内的某个成员变量的基地址 得出结构体变量的基地址
// ptr 某个成员变量的地址  type 成员变量所在的结构体  member 成员变量名称
/*
	(type *)0)->member 基于0地址获取member的偏移 也就是成员再结构体内的偏移
	然后再用ptr减去这个偏移就是结构体变量的基地址

	tip: 宏函数中 最后一个表达式的值就是返回值
*/
#define container_of(ptr, type, member)                                     \
	({                                                                      \
		typeof(((type *)0)->member) *p = (ptr);                             \
		(type *)((unsigned long)p - (unsigned long)&(((type *)0)->member)); \
	})

#define sti() __asm__ __volatile__("sti	\n\t" :: \
									   : "memory")
#define cli() __asm__ __volatile__("cli	\n\t" :: \
									   : "memory")
#define nop() __asm__ __volatile__("nop	\n\t")
#define io_mfence() __asm__ __volatile__("mfence	\n\t" :: \
											 : "memory")

// 双向链表
struct List
{
	struct List *prev;
	struct List *next;
};

// 初始化 这里初始化是都先指向自己
static inline void list_init(struct List *list)
{
	list->prev = list;
	list->next = list;
}

// add加到entry后面
static inline void list_add_to_behind(struct List *entry, struct List *add) ////add to entry behind
{
	add->next = entry->next;
	add->prev = entry;
	add->next->prev = add;
	entry->next = add;
}

// add加到entry前面
static inline void list_add_to_before(struct List *entry, struct List *add) ////add to entry behind
{
	add->next = entry;
	entry->prev->next = add;
	add->prev = entry->prev;
	entry->prev = add;
}

// 删除entry
static inline void list_del(struct List *entry)
{
	entry->next->prev = entry->prev;
	entry->prev->next = entry->next;
}

// 判断吧list是否为空
static inline long list_is_empty(struct List *entry)
{
	if (entry == entry->next && entry->prev == entry)
		return 1;
	else
		return 0;
}

static inline struct List *list_prev(struct List *entry)
{
	if (entry->prev != NULL)
		return entry->prev;
	else
		return NULL;
}

static inline struct List *list_next(struct List *entry)
{
	if (entry->next != NULL)
		return entry->next;
	else
		return NULL;
}

/*
		From => To memory copy Num bytes
*/

static inline void *memcpy(void *From, void *To, long Num)
{
	int d0, d1, d2;
	__asm__ __volatile__("cld	\n\t"
						 "rep	\n\t"
						 "movsq	\n\t"
						 "testb	$4,%b4	\n\t"
						 "je	1f	\n\t"
						 "movsl	\n\t"
						 "1:\ttestb	$2,%b4	\n\t"
						 "je	2f	\n\t"
						 "movsw	\n\t"
						 "2:\ttestb	$1,%b4	\n\t"
						 "je	3f	\n\t"
						 "movsb	\n\t"
						 "3:	\n\t"
						 : "=&c"(d0), "=&D"(d1), "=&S"(d2)
						 : "0"(Num / 8), "q"(Num), "1"(To), "2"(From)
						 : "memory");
	return To;
}

/*
	计算字符串长度
	cld df=0 控制内存地址向高位递增
	repne 当ecx!=0且zf=0时 重复执行后面的指令 执行一次ecx-1
	notl 表示not的l（long 32bit 4B版本）
	D 表示使用RDI EDI DI寄存器 "0"表示和input的第0个关联作为输入 使用想同的寄存器 这样取反等操作针对=c指定的ecx再输出到__res
	scasb scan string byte 比较al 和 byte of [ES:EDI] 如果df=0 inc edi
	执行一次ecx-1 初始时0xffffffff 所以取反再减一就是长度 notl和decl(例如两次 变成fffffffd 取反00000002 减去1（'0'本身）就是1)
*/
static inline int strlen(char *String)
{
	// register 关键字说明后面的变量访问频率较高 告诉编译器尽量将他存储在寄存器中
	register int __res;
	__asm__ __volatile__("cld	\n\t"
						 "repne	\n\t"
						 "scasb	\n\t"
						 "notl	%0	\n\t"
						 "decl	%0	\n\t"
						 : "=c"(__res)
						 : "D"(String), "a"(0), "0"(0xffffffff)
						 :);
	return __res;
}

/*
		testb 置位zf 0/1 操作数AND测试  这里分别计算是否够一个byte 是的话则je
		je 1f : 向前跳转标号1 f forword 1b 倒退到1 这样可以存在相同标号
		stosq /stosl 从rax/eax的值保存到es:edi
		rep cx不等于0执行后面一条指令
		通过test 4 2 1 与count判断是几个字节的 分别调用 stosl stosw stosb
		"0"(Count / 8) 将次数/8 放入cx系列寄存器 控制rep次数 默认是stosq
		"1"(Address) 指定了di 目标

		先默认8  按照最大的8执行赋值 剩下的慢慢判断执行 count=8*cx+n n再决定执行什么指令
*/

static inline void *memset(void *Address, unsigned char C, long Count)
{
	int d0, d1;
	unsigned long tmp = C * 0x0101010101010101UL; // 拓展到8字节
	__asm__ __volatile__("cld	\n\t"
						 "rep	\n\t"
						 "stosq	\n\t"
						 "testb	$4, %b3	\n\t"
						 "je	1f	\n\t"
						 "stosl	\n\t"
						 "1:\ttestb	$2, %b3	\n\t"
						 "je	2f\n\t"
						 "stosw	\n\t"
						 "2:\ttestb	$1, %b3	\n\t"
						 "je	3f	\n\t"
						 "stosb	\n\t"
						 "3:	\n\t"
						 : "=&c"(d0), "=&D"(d1)
						 : "a"(tmp), "q"(Count), "0"(Count / 8), "1"(Address)
						 : "memory");
	return Address;
}

/*
	8bit端口输入 inb 读取一个byte
	mfence 读写都串行化 在mfence指令前的读写操作当必须在mfence指令后的读写操作前完成
	同理还有sfence 和lfence 理解成save/load 保证写操作和读操作 指令前的写/读操作必须在指令之后的操作前完成
*/

static inline unsigned char io_in8(unsigned short port)
{
	unsigned char ret = 0;
	__asm__ __volatile__("inb	%%dx,	%0	\n\t"
						 "mfence			\n\t"
						 : "=a"(ret)
						 : "d"(port)
						 : "memory");
	return ret;
}

/*
	8bit端口输出
*/

static inline void io_out8(unsigned short port, unsigned char value)
{
	__asm__ __volatile__("outb	%0,	%%dx	\n\t"
						 "mfence			\n\t"
						 :
						 : "a"(value), "d"(port)
						 : "memory");
}

/*
	wrmsr
	address: msr index 通过ecx指定
	value: write  edx 表示高32为 eax表示低32位
*/
static inline void wrmsr(unsigned long address, unsigned long value)
{
	__asm__ __volatile__("wrmsr \n\t" ::"d"(value >> 32), "a"(value & 0xffffffff), "c"(address)
						 : "memory");
}

#endif
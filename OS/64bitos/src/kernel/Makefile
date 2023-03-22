# -S 去除符号表和relocation信息
# -R 去除指定section信息
# -O 指定输出文件格式
# objcopy [option(s)] in-file [out-file]
# 从system中提取信息生成kernel.bin
all: system
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary system kernel.bin

# 链接 -b 指定obj文件格式
# -T 指定连接脚本文件
system:	head.o entry.o main.o printk.o trap.o memory.o interrupt.o task.o
	ld -b elf64-x86-64 -z muldefs -o system head.o entry.o memory.o task.o main.o printk.o trap.o interrupt.o  -T Kernel.lds 

head.o:	head.S
# -E表示仅预处理 不执行编译汇编或者链接 预处理 编译生成汇编代码一类  汇编生成obj
	gcc -E  head.S > head.s
	as --64 -o head.o head.s
entry.o: entry.S
	gcc -E  entry.S > entry.s
	as --64 -o entry.o entry.s
# -fno-builtin 是指忽略和内建函数冲突	
# -m64 只x86-64下 设置int 32bit long pointer 64bit
# -mcmodel=large 告诉编译器 该obj文件可能被链接到大于4g的程序中 所以使用虽慢但是安全的绝对寻址？
# -c 编译汇编 但是不链接
main.o:	main.c
	gcc  -mcmodel=large -fno-builtin -m64 -c main.c

printk.o:	printk.c
	gcc  -mcmodel=large -fno-builtin -m64 -c printk.c
trap.o: trap.c
	gcc  -mcmodel=large -fno-builtin -m64 -c trap.c
memory.o: memory.c
	gcc  -mcmodel=large -fno-builtin -m64 -c memory.c
interrupt.o: interrupt.c
	gcc  -mcmodel=large -fno-builtin -m64 -c interrupt.c
task.o: task.c
	gcc  -mcmodel=large -fno-builtin -m64 -c task.c	
clean:
	rm -rf *.o *.s~ *.s *.S~ *.c~ *.h~ system  Makefile~ Kernel.lds~ kernel.bin 
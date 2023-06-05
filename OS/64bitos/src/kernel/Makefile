CFLAGS  = -mcmodel=large -fno-builtin -m64
ASFLAGS = --64
PIC = APIC
VM = VM
# -S 去除符号表和relocation信息
# -R 去除指定section信息
# -O 指定输出文件格式
# objcopy [option(s)] in-file [out-file]
# 从system中提取信息生成kernel.bin
all: system
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary system kernel.bin

# 链接 -b 指定obj文件格式
# -T 指定连接脚本文件
# -b target 指定输入文件的文件格式
# -z muldefs 允许重复定义 遇见重复定义的时候 只使用第一个
# 总体ld命令中式按照文件顺序一次链接的 所以head.o 需要作为第一个才能放到lds文件中指定的位置

ifeq (${PIC},APIC)
system:	head.o entry.o APU_boot.o main.o printk.o trap.o memory.o interrupt.o PIC.o task.o cpu.o keyboard.o mouse.o disk.o SMP.o time.o HPET.o softirq.o timer.o schedule.o
	ld -b elf64-x86-64 -z muldefs -o system head.o entry.o APU_boot.o memory.o  main.o printk.o trap.o interrupt.o PIC.o task.o cpu.o keyboard.o mouse.o  disk.o SMP.o  time.o softirq.o HPET.o timer.o  schedule.o -T Kernel.lds 
else
system:	head.o entry.o  main.o printk.o trap.o memory.o interrupt.o PIC.o task.o cpu.o time.o HPET.o softirq.o timer.o schedule.o
	ld -b elf64-x86-64 -z muldefs -o system head.o entry.o memory.o  main.o printk.o trap.o interrupt.o PIC.o task.o cpu.o time.o  HPET.o softirq.o timer.o  schedule.o -T Kernel.lds 
endif
head.o:	head.S
# -E表示仅预处理 不执行编译汇编或者链接 预处理 编译生成汇编代码一类  汇编生成obj
	gcc -E  head.S > head.s
	as $(ASFLAGS) -o head.o head.s
entry.o: entry.S
	gcc -E  entry.S > entry.s
	as $(ASFLAGS) -o entry.o entry.s
APU_boot.o: APU_boot.S
	gcc -E APU_boot.S > APU_boot.s
	as $(ASFLAGS) -o APU_boot.o APU_boot.s
# -fno-builtin 是指忽略和内建函数冲突 除非使用前缀__builtin_明确引用，否则编译器不识别所有系统内建函数 例如 alloca 等
# -m64 只x86-64下 设置int 32bit long pointer 64bit 或者说生成64位代码
# -mcmodel=large 告诉编译器 该obj文件可能被链接到大于4g的程序中 所以使用虽慢但是安全的绝对寻址？
# -c 编译汇编 但是不链接
# -D 定义宏 宏名紧跟D 等价于再程序源文件中编写代码 #define APIC
# 如果想赋值则 -DAPIC="\hello"\ 等价于源文件 #define APIC "hello"
main.o:	main.c
	gcc  $(CFLAGS) -c main.c -D${PIC}

printk.o:	printk.c
	gcc  $(CFLAGS) -c printk.c
trap.o: trap.c
	gcc  $(CFLAGS) -c trap.c
memory.o: memory.c
	gcc  $(CFLAGS) -c memory.c
interrupt.o: interrupt.c
	gcc  $(CFLAGS) -c interrupt.c
ifeq ($(PIC),APIC)
PIC.o: APIC.c
	gcc  $(CFLAGS) -c APIC.c -o PIC.o -D${VM}
else
PIC.o: 8259A.c
	gcc  $(CFLAGS) -c 8259A.c -o PIC.o
endif
task.o: task.c
	gcc  $(CFLAGS) -c task.c
keyboard.o: keyboard.c
	gcc  $(CFLAGS) -c keyboard.c
mouse.o: mouse.c
	gcc  $(CFLAGS) -c mouse.c
cpu.o: cpu.c
	gcc  $(CFLAGS) -c cpu.c		
disk.o: disk.c
	gcc  $(CFLAGS) -c disk.c	
SMP.o: SMP.c
	gcc  $(CFLAGS) -c SMP.c
time.o: time.c
	gcc  $(CFLAGS) -c time.c
HPET.o: HPET.c
	gcc  $(CFLAGS) -c HPET.c
softirq.o: softirq.c
	gcc  $(CFLAGS) -c softirq.c
timer.o: timer.c
	gcc  $(CFLAGS) -c timer.c
schedule.o: schedule.c
	gcc  $(CFLAGS) -c schedule.c
clean:
	rm -rf *.o *.s~ *.s *.S~ *.c~ *.h~ system  Makefile~ Kernel.lds~ kernel.bin 
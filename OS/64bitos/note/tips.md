# 途中遇到的各类和正文不相关的基础问题

- [linux](#linux)
- [bochs](#bochs)
- [汇编](#asm)
- [链接](#link)
- [c语言](#c)



<div id ="linux"><h2>linux</h2</div>

### 分区 挂载硬盘

```shell

#查看 
sudo fdisk -l
lsblk -f

# 分区
sudo fdisk /dev/xxx

#然后根据提示操作  比如 d n等 记得 最后w才是写入生效 不w就可以重新来


# 格式化 ntfs ext4等等

mkfs -t ntfs xxx
mkfs.ntfs xxx
```

### 各类地址的理解

可以参见advance(cpu-architecture)中的说明

虚拟地址包含逻辑地址和线性地址 反正理论上不应该直接访问物理设备的

逻辑地址  主要是偏移地址 相当于程序给的 结合段选择子或者段地址变成线性地址



线性地址  如果没有分页可能就是物理地址 分页就是虚拟地址

或者说对应分段和分页两种形式

分页的话 线性地址 通过cr3 读取目录页 直接转换为物理地址 逻辑还是offset  

分段的话  线性地址或者也可以成为物理地址 



物理地址 实际地址


几个控制寄存器作用

CR0 是系统内的控制寄存器之一。控制寄存器是一些特殊的寄存器，它们可以控制CPU的一些重要特性。
 
CR1是未定义的控制寄存器，供将来的处理器使用。
CR2是页故障线性地址寄存器，保存最后一次出现页故障的全32位线性地址。
CR3是页目录基址寄存器，保存页目录表的物理地址，页目录表总是放在以4K字节为单位的存储器边界上，因此，它的地址的低12位总为0，不起作用，即使写上内容，也不会被理会。
CR4在Pentium系列（包括486的后期版本）处理器中才实现，它处理的事务包括诸如何时启用虚拟8086模式等


### 用户空间 内核空间 用户态 内核态

- **用户空间和内核空间:** 首先这是基于虚拟地址的，当然也可以简单理解成物理地址。内核程序存储运行的相关内存空间就是内核空间 一般是高位的地址。剩下的就是用户空间。比如32bit 一般是高位1g作为内核空间，剩下的3g就是用户空间 这时候一个进程最多可以申请3g的内存空间。
- **用户态和内核态:** 每个进程都有两个状态 用户态就是自己代码执行的状态  内核态则是例如中断或者进程切换等情况下陷入内核态 区别是执行的特权等级



<div id="bochs"><h2>bochs</h2></div>

### make错误

参见[这篇文章](https://www.cnblogs.com/oasisyang/p/15358137.html)


<div id ="asm"><h2>汇编</h2</div>

### 寄存器补充知识

rax eax ax al/ah 其实应该算是一个寄存器。为了兼容以及拓展
rax代表64位寄存器。eax代表32，ax 16  ah，al则是ax的高低8位


.S文件，会进行预处理、汇编等操作。

.s文件，在后期阶段不在进行预处理操作，只有汇编操作。

.asm文件，等同于.s文件。因为汇编本质上是纯文本的，不管用什么后缀都可以。所以一般dos和windows下以.asm为主，Intel指令 NASM编译 linux下以.s为 主 基于AT&T指令，采用GAS编译


GAS 伪指令     https://sourceware.org/binutils/docs/as/Pseudo-Ops.html#Pseudo-Ops
 
ELF Version
.section name [, "flags"[, @type[,flag_specific_arguments]]]
.section指示把代码划分成若干个段（Section），程序被操作系统加载执行时，每个段被加载到不同的地址，操作系统可以对不同的段设置不同的读、写或执行权限。每一个段以段名为开始，以下一个段名或者文件结尾为结束。

对于ELF文件格式来说，flags参数（注意使用的时候需要将参数包在一对双引号之间）有以下几种取值，且可以按照任何组合使用：

a：该段可被分配。
w：该段可写。
x：该段可执行。
M：该段可被合并（如果有别的段和这个段属性一样的话）。
S：该段包含“\0”结尾的字符串。
G：该段是某个段组中的一员。
T：该段被用于线程本地化存储（TLS，Thread Local Storage）
等等

Note - some sections, eg .text and .data are considered to be special and have fixed types. Any attempt to declare them with a different type will generate an error from the assembler.

.text .data是特定的段 具有固定的类型 所以不能定义不同的类型 使用默认的即可

subsection name 就是子段  属于段下面的



 例如：

.section .text        #定义文本段（代码段） 对进程而言是只读的

.section .data        #定义数据段   读写的已经初始化的数据 

.section .bss          #定义 bss 段 未初始化的读写数据（Block Started by Symbol）


.section ".vectors", "ax"       #指定以下代码段必须存放在.vectors段里， “ax”表示该段可执行并且可分配
————————————————


.section 或者segment关键字定义节  链接的时候将属性相同的节合成大段 Segment
目标文件中式以节组织的  和一个section header table中一一对应
而文件载入内存执行时，是以segment组织的，每个segment对应ELF文件中program header table中的一个条目，用来建立可执行文件的进程映像。
比如我们通常说的，代码段、数据段是segment，目标代码中的section会被链接器组织到可执行文件的各个segment中。
.text section的内容会组装到代码段中，.data, .bss等节的内容会包含在数据段中。



att寻址语法 
offset(base,index,factor)

偏移(%基址寄存器,%索引寄存器,%比例因子) #所有字段都是可选的
偏移以及比例因子必须是常量,其余的两个必须是寄存器,如果省略任何一项将会默认为0.


最终地址 base+index*factor+offset



<div id ="link"><h2>链接</h2></div>


```lds
SECTIONS
{
. = 0x10000;
.text : { *(.text) }
. = 0x8000000;
.data : { *(.data) }
.bss : { *(.bss) }
}
```

每个段前的`.=xxx`表示设置段的地址。 `.`相当于是位置计数器。输出段的地址就是位置计数器的值SECTIONS 开始位置计数器默认是0。并且累加后面段的长度。如果设置了 那么就是指定了我们需要的值。例子里代码段 `.text`被指定到默认0x10000的位置 也就是1M的位置。

`*` 是通配符 代表所有目标文件的代码段都放到可执行文件输出段的代码段.text中。


SECTIONS。SECTIONS关键字负责向链接器描述如何将输入文件中的各程序段（数据段、代码段、堆、栈、BSS）部署到输出文件中，同时还将规划各程序段在内存中的布局。

.text程序段还使用了_text和_etext标识符来标示.text程序段的起始线性地址和结尾线性地址，这两个标识符可在程序中通过代码extern _text和extern _etext进行引用（将它们看作全局变量）。其他程序段同理。此处的符号.表示程序定位器的当前位置（线性地址）。


OUTPUT_FORMAT(DEFAULT,BIG,LITTLE)。它为链接过程提供DEFAULT（默认）、BIG（大端）、LITTLE（小端）三种输出文件格式（文件格式请参见ld命令的-b TARGET选项）。在程序的链接过程中，若链接命令使用-EB选项，那么程序将链接成BIG指代的文件格式；如果链接命令中有-EL选项，那么程序将链接成LITTLE指代的文件格式。否则程序将链接成默认文件格式，即DEFAULT指代的文件格式。


ALIGN(NUM)。它将地址向后按NUM字节对齐。


ENTRY(SYMBOL)：将符号 SYMBOL 的值设置为入口地址，入口地址是进程执行的第一条指令在进程地址空间的地址（比如 ENTRY(Reset_Handler) 表示进程最开始从复位中断服务函数处执行）

 有多种方法设置进程入口地址，以下编号越小，优先级越高

1. ld 命令行的 -e 选项
2. 链接脚本的 ENTRY(SYMBOL) 命令
3. 在汇编程序中定义了 start 符号，使用 start 符号值
4. 如果存在 .text section，使用 .text section 首地址的值
5. 使用地址 0 的值

在x64模式下，大部分编译器采用寄存器传递参数，参数按照从左向右的顺序依次是RDI、RSI、RDX、RCX、R8、R9，剩余参数使用内存传递方式，RAX放置函数的返回值，调用者负责平衡栈。

%rax ：通常存储函数调用的返回结果，也被用在idiv （除法）和imul（乘法）命令中。



<div id ="c"><h2>c语言</h2></div>

1. c语言结构体中不可以包含函数 但是能够包含指针 所以可以包含函数指针 或者指针的指针
2. c语言参数传递时值传递或者引用传递
    - 值传递代表的就是复制 比如int类型 他们在内存的位置不一致 只是值一致
    - 指针也是一样 相当于传递了一个值 那里面的地址时一致的。
        ```c
         #include <stdio.h>

                int main()
                {
                    char a = 'a';
                    char *str = &a;
                    // a= 0000000000000061 &a = 000000000061FE1F, str=000000000061FE1F,*str=0000000000000061,&str=000000000061FE10        
                    printf("a= %p &a = %p,str=%p,*str=%p,&str=%p", a, &a,str, *str, &str);
                    return 0;
                }
        ```
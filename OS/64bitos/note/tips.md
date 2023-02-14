# 途中遇到的各类和正文不相关的基础问题

- [linux](#linux)
- [bochs](#bochs)
- [汇编](#asm)



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
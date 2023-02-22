# 内核层开始

包含内容
- [1.内核执行头程序](#c1-1)
    -  [1.1 内核头程序简述](#c1-1)
    -  [1.2 x64分页机制简述](#c1-2)
    -  [1.3 Makefile简介](#c1-3)
- [2.环境搭建以及基础知识](#c2)
    - [2.1 c与汇编](#c2-1)
        - [汇编调用c约定](#c2-1-1)
        - [c内嵌汇编](#c2-1-2)
- [3.BootLoader引导启动程序](#c3)
    - [3.1 Boot引导程序](#c3-1)
        - [FAT12文件系统](#c3-1-1)
    - [3.2 Loader程序](#c3-2)
    - [3.3 实模式到保护模式到IA-32e模式](#c3-3)
        - [实模式到保护模式](#c3-3-1)
        - [保护模式到IA-32e模式](#c3-3-2)



TIP:(开始使用AT&T格式的GAS汇编语言)
<div id=c1><h2>内核执行头程序</h2></div>
<div id=c1-1><h3>内核头程序简述</h3></div>

内核头程序:当引导程序加载完成并移交控制权后，还需要内核头程序为操作系统创建段结构和页表结构 设置某些结构的默认处理函数 配置关键寄存器等工作。然后才继续执行一个远跳转进入系统内核主程序。其中boot,loader,头程序，内核主程序地址空间位置如图:

<img src="./img/kernel_header.png">

这里需要注意的是确保头程序在编译生成到内核程序的开头处。这就需要手动编写内核程序的链接脚本。此处记住线性地址的0xffff80000000000对应物理地址的0处 然后内核程序的线性地址就是0xffff80000000000+0x100000处即可(TODO)

<div id=c1-2><h3>x64分页机制简述</h3></div>

转自 [x64四级分页机制](https://www.cnblogs.com/revercc/p/16041279.html)

简单流程

cr3->PML4T(page map lv4 table )->PDPT(page-directory-pointer table)->PDT(page directory talbe)->PT (page table )
cr3直接找到PML4T的物理地址 然后根据l4 index找到PDPT 然后根据l3index 找到PDT 然后根据l2index找到 PTT 然后根据l1index 在PTT中找到实际物理帧地址


因为硬件限制64位系统只使用64位虚拟地址的低48位，48位虚拟地址被分为9-9-9-9-12，4个9分别表示PML4T,PDPT,PDT,PTT的PFN页帧编号(与物理页帧区分)
（这里offset12 代表的是4kib的分页 如果3j自然是变为9+12=21 offset 那就是2Mb 同理还有1GB）
线性地址/虚拟地址结构如下（也可以参见blogos中paging-introducation.md）:

<img src="./img/linearAddress.png"><br>
其中还有置零的16位符号拓展位略  cr3指向的就是PML4T cr3得12-35bit是物理地址的高24 低12位置零。

如图:

<img src="./img/cr3.png"><br>

PML4E(page map lv4 entry)的35-12位为PDPT页目录指针表物理地址的高24位，低12位置0。如图（全图）

<img src="./img/page_address.png">


这里如果PDPTE的PS [page size?]（bit[7]）位置1表示开启1G大页 那么就没有PDT和PTE了。这里的35-30bit 就直接是物理地址的高6位 低30位置零。偏移地址就是9+9+12的值 提下每个bit[0]或者present位表示页表或物理页是否加载到主内存中。 基本都是置1
同理 如果PDTE的PS置1则是开2MB中页 那就没有PT了 他的35-21bit是物理地址的高15位。低21位置零 offset 9+12。不然 35-12位是PT的物理地址。

最后PTE的35-12位是物理内存的起始地址

这里补充下 实模式/16bit 到保护模式/32bit(这里可能有IA-32) 到IA-32e/bit64模式的寻址变化

16bit base*16+offset 32bit 段寄存器里放的是选择子 然后再去描述符表找base。（可能存在4GB平坦内存寻址） 64位基本放弃分段 段寄存器无论是什么 base 都视为零 。这里是方便记忆理解 不一定准确。比如还有PAE的寻址模式这些。

<div id=c1-3><h3>Makefile简介</h3></div>

```makefile
# Makefile for boot
# Programs, flags, etc.
ASM             = nasm
ASMFLAGS        = -I include/# This Program
TARGET          = boot.bin loader.bin# All Phony Targets
.PHONY : everything clean all# Default starting position
everything : $(TARGET)

clean :
    rm -f $(TARGET)

all : clean everything

boot.bin : boot.asm include/load.inc include/fat12hdr.inc
    $(ASM) $(ASMFLAGS) -o $@ $<

loader.bin : loader.asm include/load.inc include/fat12hdr.inc include/pm.inc
    $(ASM) $(ASMFLAGS) -o $@ $<
```

这里一个实例。然后分开看看。
首先`#`开头的行时注释 `=`定义变量。下面的`ASM`和`ASMFLAGS`就是变量。使用的时候要用`$(ASM)`的形式。

```makefile
ASM             = nasm
ASMFLAGS        = -I include/# This Program
TARGET          = boot.bin loader.bin# All Phony Targets
```


Makefile最重要的语法：
```makefile
target : prerequisites
    command
```
这个代表两个意思：

1. 想要得到target 需要执行命令command(缩进tab)
2. target 依赖 prerequisites.当prerequisites中至少有一个文件比target文件新时，command才被执行。

举个例子:

```makefile
loader.bin : loader.asm include/load.inc include/fat12hdr.inc include/pm.inc
        $(ASM) $(ASMFLAGS) -o $@ $<
```
这段代码的意思就是想要得到`loader.bin`需要执行`$(ASM) $(ASMFLAGS) -o $@ $<`这段命令。然后`loader.bin`依赖一下文件：

- loader.asm
- include/load.inc
- include/fat12hdr
- include/pm.inc

这四个文件有一个更新时 command会被执行。

然后`$@`和`$<`的含义如下:

- `$@`代表target
- `$<`代表prerequisites的第一个名字

那么结合使用变量的语法，`$(ASM) $(ASMFLAGS) -o $@ $<`翻译过来就是`nasm -I include/ -o loader.bin loader.asm`

然后时这部分语法

```makefile
everything : $(TARGET)

clean :
        rm -f $(TARGET)

all : clean everything
```

这代表三个指令动作,对应 `make clean` 就是执行`rm -f boot.bin loader.bin`。`make all`就是分别执行 `make clean`然后执行`everything`。`everything`其实就是生成两个bin目标项

然后是`.PHONY`这个显式声明几个动作标号，这里就是`everything clean all`这三个(不代表仅有这三个 这个是问了防止重名的)。然后`make`默认执行的就是第一个遇到的标号，这里就是默认执行`everything`的动作。和`.PHONY`里定义的顺序无关。参见下面这个例子

```makefile
ECHO=echo
goal=test
PRE=pre
TARGET=pre test
.PHONY: everything first clean all
zero:
	echo "zero"
first:
	$(ECHO) "123" > $(PRE)
everything: $(goal)
clean:
	rm -rf $(TARGET)

all: clean first everything

test:pre
	cat pre > $@
```

这里默认执行的是`zero` 也就是执行`echo "zero"`


# 源代码到可执行文件


包含内容
-  [1 编译](#c1)
    - [编译流程](#c1-1)
    - [GCC 编译实例](#c1-2)
-  [2 ELF文件](#c2)
    - [ELF文件结构](#c2-1)

</br></br></br></br>
<div id=c1><h2>编译原理</h2></div>


编译器功能主体如下图:

<img src="../image/compile.png"/>

<div id=c1-1><h3>编译流程</h3></div>
其中编译基本步骤如下图:

<img src="../image/compile_step.png"/>

1. 这边先读取源程序的`字符流`，编译器根据代码生成词素(lexeme),这里的词素就是代码中最小单位，不能继续分割 。举个例子:
    ```c
    a = b+c*60;
    ``` 
    这里生成的词素就是`a`, `=`, `b`, `+`, `c`, `*`,`60`,`;`.其中主要分为`保留字`,`标识符`,`常数`,`运算符`,`界符`。这里`界符`就是类似空白符，分号，括号等.然后根据这些词素会生成token表.比如`<id1,position>, <op1, =>`等。这些`id1`和`op1` 是一个示例 具体看实现。比如标识符的生成规则以及部分运算符定义。特别是标识符这边比如通过类似字母+数字的格式 也可以区分不同作用域的同名标识符。

2. 生成token也就是符号流之后会进行语法分析创建语法树
3. 然后根据符号表以及语法树判断是否符合语言规范并且收集类型想信息供后面作类型转换优化等。
4. 接下里就是生成对应语言的中间代码表示 例如三地址码。
5. 对中间代码进行优化
6. 根据优化后的代码根据目标机器生成对应的机器语言并优化。

这里提下三地址码：是一种常见的中间语言，类似汇编语言。每个指令组多存在三个操作数 所以成为三地址码。简单举个例子 `x=a op b` 这里就是最多的三个操作数a,b,和结果x.当然还有类似`x=y`的2操作数，`if x rop y goto l`则是如果`x relationop y` 则跳转l或者`call x,n`的形式，这里就是调用x过程，n为参数个数。三地址码可以通过四元式来表示(运算符,操作数1,操作数2,结果)。其中不足的用下划线表示。比如`(call,x,n,_)`.


<div id=c1-2><h3>GCC 编译实例</h3></div>


gcc编译主要分为 `预处理`,`编译` `汇编` `连接`四个主要步骤。

`预处理`这里通过`gcc hello.c -o hello -save-temps --verbose`保留中间文件并且打印详细信息。其中核心三条信息:
- `/usr/lib/gcc/x86_64-linux-gnu/10/cc1 -fpreprocessed hello.i ...(略过) -o hello.s` 这里通过`cc1`编译器实现了预处理以及编译两个步骤生成`hello.s`编译文件。其中还有个中间文件`hello.i`这是预处理后的文件。说明见`code/base/hello_test/hello.c`.

简单说明如下:

- 头文件展开：将源代码中以#include开头的行替换为对应的头文件内容。
- 宏替换：将源代码中以#define开头的行所定义的宏进行替换。
- 条件编译处理：根据源代码中以#if、#ifdef、#ifndef等开头的行对代码块进行条件编译处理。
- 注释处理：将源代码中的注释删除。
- 添加行号和文件名标识?

`编译` gcc我这边是17 默认AT&T汇编语法

`汇编`: `as -v --64 -o hello.o hello.s` `as`工具 根据机器指令和汇编代码对照表生成目标文件.此时还没有链接 因为虚拟地址未定。可以objdump查看

`链接`: ` /usr/lib/gcc/x86_64-linux-gnu/10/collect2 -plugin ...(略过) -o hello`。`collect2`命令。分为静态链接和动态链接两种。默认动态链接。这边包括地址和空间分配，符号绑定和重定位等操作。也即是分配虚拟地址以及对应页基址(分页内存管理下)。

<div id=c2><h2>ELF文件</h2></div>

elf（executable and linkable file）文件是linux下的运行文件格式。相关定义在`usr/include/elf.h`中。

elf分为四种文件：

- 可执行文件(.exec): 经过链接的可执行的目标文件。
- 可重定位文件(.rel): 经过预处理编译以及汇编三个阶段但是没有链接的目标文件。通常`.o`为扩展名。可以和其他目标文件链接构成可执行文件或者动态链接库。通常是一段位置独立的代码。`gcc -c`就是编译汇编但是不链接
- 共享目标文件(.dyn): `gcc -c -fPIC elfDemo.c -o elfDemo_pic.rel && gcc -shared elfDemo_pic.rel -o elfDemo.dyn`。动态链接库文件。可以和其他动态链接库文件或者可重定位文件构建可执行文件，也可以在其他可执行文件加载时链接到进程中作为运行代码的一部分。
- 核心转储文件(Core Dump file):进程意外终止时进程地址空间的转储。

<div id=c2-1><h3>ELF文件结构</h3></div>


这里有个两个视角:

<img src="../image/elf_link.svg"/>



**先从链接视角**:

链接视角通过节 section 来划分。ELF通常都会包含3个固定的节:

- .text: 代码节。保存可执行文件的机器指令
- .data: 数据节。保存已经初始化的全局变量(包含静态变量)和局部静态变量
- .bss:  BSS节。保存未初始化的全局变量和局部静态变量

 一般指令节对京城讲是只读的。数据段是读写的。


简化版本的结构:

| ELF File       |
| :------------- |
| file header    |
| .text  section |
| .data  section |
| .bss   section |

除了上面三个节外，还有个文件头。位于elf文件的最开始的位置。开头是一个固定标识: `7f 45 4c 46` 也就是`\177ELF`(\177 是DEL的8进制) 然后是文件的相关信息。比如文件类型 版本/ABI版本 目标机器 程序入口 段表和节表的位置和长度等。

`readelf -h elfDemo.rel`


<img src="../image/elf_header_ex.png"/>

结构代码如下:

```c
typedef struct {  
    unsigned char e_ident[EI_NIDENT]; /* 文件标识和类型 */  
    Elf64_Half e_type;                /* 文件类型 */  
    Elf64_Half e_machine;             /* 目标文件体系类型 */  
    Elf64_Word e_version;             /* 文件版本 */  
    Elf64_Addr e_entry;               /* 入口点虚拟地址 */  
    Elf64_Off e_phoff;                /* 程序头部表偏移 */  
    Elf64_Off e_shoff;                /* 节区头部表偏移 */  
    Elf64_Word e_flags;               /* 处理器特定标志 */  
    Elf64_Half e_ehsize;              /* ELF 头部大小 */  
    Elf64_Half e_phentsize;           /* 程序头部表项大小 */  
    Elf64_Half e_phnum;               /* 程序头部表项数量 */  
    Elf64_Half e_shentsize;           /* 节区头部表项大小 */  
    Elf64_Half e_shnum;               /* 节区头部表项数量 */  
    Elf64_Half e_shstrndx;            /* 包含节名称的字符串表的索引 */  
} Elf64_Ehdr;
```

这边大部分可以直接看懂，最后一个`e_shstrndx`是`shstrtab`在节表中的索引号。比如这边就是12.然后`.shstrtab`仅仅是一个字符串数组，里面仅仅是空字符和字符串(section_name).

下面时`elfDemo.rel`中读取`section header table`的实例:


`readelf -S elfDemo.rel`


<img src="../image/read_elf_sh.png" />

可以看到`offset`时`0x450` 也就是`1104` 和上图中的`start of section headers`一致。`13`和`numbers of section headers` 一致。


然后就是`section header table`中每个`entry`的结构

```c
typedef struct {  
    Elf64_Word  sh_name;       /* Section name (string tbl index) */  
    Elf64_Word  sh_type;       /* Section type */  
    Elf64_Xword sh_flags;      /* Section flags */  
    Elf64_Addr  sh_addr;       /* Section virtual addr at execution */  
    Elf64_Off   sh_offset;     /* Section file offset */  
    Elf64_Xword sh_size;       /* Section size in bytes */  
    Elf64_Word  sh_link;       /* Link to another section */  
    Elf64_Word  sh_info;       /* Additional section information */  
    Elf64_Xword sh_addralign;  /* Section alignment */  
    Elf64_Xword sh_entsize;    /* Entry size if section holds table */  
} Elf64_Shdr;
```

这边举例看下`.text`部分


| Name  | Type     | Address          | Offset   | Size             | EntSize          | Flags | Link | Info | Align |
| :---- | :------- | :--------------- | :------- | :--------------- | :--------------- | :---- | :--- | :--- | :---- |
| .text | PROGBITS | 0000000000000000 | 00000040 | 0000000000000050 | 0000000000000000 | AX    | 0    | 0    | 1     |

说明大概就是.text section。类型是`PROGBITS`代表程序的相关信息。还有比如`NULL`表示该节不使用 `STRTAB`表示节区包含字符串表，用于存储符号名和其他字符串等。

因为没有经过链接，所以address都是0.然后offset实际就是该段的长度。这边是`0x50` 类型是`AX`则是可以执行(X)并且节区的内容会被加载到内存中(A)。

这边发现`objdump`指令无法读取部分section 还是可以通过`readelf -p index/section_name elf_name`来实现。这边部分隐士创建的比如`.got`等节区可能无法在`shstrtab`中找到。

`readelf -p .shstrtab elfDemo.rel`

```
String dump of section '.shstrtab':
  [     1]  .symtab
  [     9]  .strtab
  [    11]  .shstrtab
  [    1b]  .rela.text
  [    26]  .data
  [    2c]  .bss
  [    31]  .rodata
  [    39]  .comment
  [    42]  .note.GNU-stack
  [    52]  .rela.eh_frame
```














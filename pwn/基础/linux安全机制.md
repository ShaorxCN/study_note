# linux安全机制

-  [1 基础知识](#c1)
    - [部分指令说明](#c1-1)
    - [部分概念说明](#c1-2)
    - [linux安全机制](#c1-3)
      - [Stack Canaries](#c1-3-1)
      - [No-eXecute](#c1-3-2)




<div id=c1-1><h2>部分指令说明</h2></div>


- 0 stdin 标准输入
- 1 stdout 标准输出
- 2 stderr 标准错误
- cmd>file 覆盖重定向
- cmd >> file 追加重定向
- cmd < tag 标准输入中读取 直到遇到tag
- cmd < file1 > file2 file作为cmd的标准输入并且输出到file2
- cmd 2 > file 将cmd的标准错误重定向并覆盖file
- 2 >& 1 将标注错误和标准输出合并

<div id=c1-2><h2>部分概念说明</h2></div>

linux 三种基本文件类型

1. 普通文件: 文本文件和二进制文件(所有其他文件)
2. 目录: 包含一组链接的文件 每个链接都将文件名映射到一个文件。这个文件也可能是目录
3. 特殊文件: 块文件 符号链接 管道 套接字等


/etc/group gid
/etc/passwd /etc/shadow

所有者-组权限-其他人权限

所有者u 所属组 g 其他人o 所有人a

chmod a+r file 增加所有人读权限
chmod u-w file 删除所有者写权限
chmod g=rwx file 指定所属组权限读写执行

当前终端下export 临时环境变量
修改配置文件的成为永久环境变量


/etc/profile 系统环境变量
~/.bashrc 用户环境变量


LD_PRELOAD 可以定义程序运行时优先加载的动态链接库。你可以通过设置 LD_PRELOAD 来改变程序对特定函数的调用，使其优先链接到你指定的共享库中的同名函数，而不是标准的库函数。

```
LD_PRELOAD=~/libc-2.23.so
```

这样程序就会优先加载当前用户目录下2.23的libc


environ 是libc中的全局变量  指向内存中的环境变量表。该表位于栈上。 linux下使用 pmap指令。

**procfss文件系统**

procfs（进程文件系统）是一种特殊的文件系统，它提供了一个以文件和目录的形式来表示正在运行的进程信息的接口。它通常被挂载在 /proc 目录下，允许用户和系统管理者访问进程相关的信息和状态。

在 Linux 系统中，procfs 提供了一个虚拟文件系统，它反映了内核中运行的进程和系统的当前状态。通过访问 /proc 目录下的文件和目录，您可以获取和修改进程的各种信息，例如进程的状态、命令行参数、打开的文件、内存映射、网络连接等。

以下是一些常见的在 /proc 目录中查看进程信息的文件和目录：
```
/proc/<PID>/status：包含有关进程的基本信息，如进程状态、父进程、线程数等。
/proc/<PID>/cmdline：包含进程的完整命令行参数。
/proc/<PID>/fd：代表进程打开的文件描述符。
/proc/<PID>/maps：列出了进程的内存映射信息，包括各个区域的起始地址、大小和权限等。
/proc/<PID>/net：提供有关进程网络连接的信息。
/proc/<PID>/auxv:传递给进程的解释器信息
/proc/<PID>/stack:进程的内核栈信息
/proc/<PID>/task:进程的线程信息
/proc/<PID>/maps:进程的内存信息
```
除了上述文件和目录，procfs 还提供了其他许多与进程相关的文件和目录，它们以进程的 PID 命名。您可以通过访问这些文件和目录来获取所需的进程信息。

**字节序**

大端序和小端序。x端序就是x位在内存的地位。大端序就是高位存储在内存的地位。针对byte级别的。如果字双字节 就按照这个重新排序。比如0x123456.这边大端序在内存中就是0x12 0x34 0x56 小端序则是0x56 0x34 0x12.


**调用约定**

- 内核接口
  - x86-32:
    1. 系统调用号：要调用的系统调用通过存储在 EAX 寄存器中的系统调用    号（syscall number）来指定。
    2. 参数传递：系统调用的参数传递通常使用寄存器进行。常见的参数传 递寄存器包括：EBX、ECX、EDX、ESI 和 EDI。具体使用哪些寄存器取决  于系统调用的参数数量和类型。
    3. 调用指令：使用 int 0x80 指令触发软中断来执行系统调用。执行该 指令时，CPU 会将寄存器中的重要数据保存并切换到内核模式，在内核中 处理系统调用。
    4. 返回值：系统调用完成后，返回值通常会存储在 EAX 寄存器中，以供    用户程序进行进一步处理。
  - x86-64:
    1. 系统调用号：要调用的系统调用通过存储在 RAX 寄存器中的系统调用号（syscall number）来指定。不同于 x86-32 架构，x86-64 使用更广泛的 RAX 寄存器来存储系统调用号。
    2. 参数传递：系统调用的参数传递通常使用寄存器进行。常见的参数传递寄存器包括：RDI、RSI、RDX、R10、R8 和 R9。具体使用哪些寄存器取决于系统调用的参数数量和类型。如果参数数量超过传递寄存器的数量，额外的参数将使用栈传递。
    3. 调用指令：使用 syscall 指令触发一个软中断来执行系统调用。执行该指令时，CPU 会将寄存器中的重要数据保存并切换到内核模式，在内核中处理系统调用。
    4. 返回值：系统调用完成后，返回值通常会存储在 RAX 寄存器中，以供用户程序进行进一步处理。
- 用户接口
    - x86-32:使用栈传递参数。参数由右到左的顺序入栈。然后call执行
    - x86-64:通过寄存器传递参数.也会使用栈，如果参数是memory类型。则通过栈传递参数。如果是INTEGER则寄存器。RDI、RSI、RDX、RCX、R8 和 R9
  
**核心转储**

使用ulimit设置。

这边root用户可以修改coredump部分设置 比如存储路径 文件名等。wsl或者部分虚拟机测试无法成功 需要查看相关文档

```
echo 1 > /proc/sys/kernel/core_uses_id 核心存储文件名改为core.[pid]

echo /tmp/core-%e-%p-%t > /proc/sys/kernel/core_pattern 这里修改存储路径以及文件名。core-程序名(不包含路径)-pid-时间戳
```


**系统调用**



```
见代码ch1/system_call/hello.S
---- strace ./hello64


execve("./hello64", ["./hello64"], 0x7ffff620fe00 /* 15 vars */) = 0
write(1, "hello 64-bit!\n", 14hello 64-bit!
)         = 14
exit(0)                                 = ?
+++ exited with 0 +++
```

可见使用了三个系统调用 execve write 以及exit

一般我们使用用户空间实现的api而不是直接使用系统调用。比如我们用`printf()->c库的printf()->c库的write()->write系统调用`




<div id=c1-3><h2>linux安全机制</h2></div>

<div id=c1-3-1><h3>Stack Canaries</h3></div>

栈保护技术 防止缓冲区溢出攻击的安全机制。主要是将一个特殊的随机值插入到函数的栈帧中。在函数返回的时候 去检测这个值是否变化。

主要是函数返回之前，一般在调用函数的时候 返回地址会压入被调用函数的栈。一般是最后压入的。这时候如果通过栈溢出的方式就可以修改函数返回的地址。canary就是保护这个的，他一般会在返回地址的后面(高地址) 也就是先于返回地址入栈，在局部变量空间的前面(低地址处)。也就是两者之间。这样如果通过栈溢出的方式覆盖返回地址必然会覆盖canary值。局部变量溢出的时候向下必然先覆盖canary值，这样通过检查canary值是否被篡改就可以判读是否发生了栈溢出攻击。如下图。这边局部变量是在prologue  函数序言部分入栈的。这边可以参考OS/64bitos/

```
+-------------------+
|                   | <-- High Memory Address
|       ...         |
|                   |
|-------------------| 
|       argn        |  
|-------------------|
|       ...         |
|                   |
|-------------------|
|       arg1        |
|-------------------|
|   return addr     |
|-------------------|
|  stack canary     |
|-------------------| <-- 调用者栈帧底部 (rbp) 被调用者顶部
|     old rbp       |  
|-------------------|
|    local var      |
|-------------------|
|       ...         |
|                   |
|-------------------| <-- Low Memory Address rsp



```


这边有个简单的例子。细节不准确 顺序类似

```assembly
....
push return_address
push canary
push rbp
mov rbp,rsp
sub rsp,<局部变量空间大小等> 
...
```

这里可以理解rsp一直在操作，但是通过rbp来锁定函数栈帧，比如这边push rbp 就是保存caller的函数栈帧之后mov rbp,rsp就是指定了callee的栈帧。然后再在rsp的基础上为callee分配局部变量空间。返回的时候再通过 mov rsp,rbp逻辑释放callee的函数栈帧。 pop rbp ,回复caller的rbp 然后ret.



Canaries 分为三类

1. Terminator canaries:因为很多栈溢出是字符串操作导致。所以这边将低位直接设置为类似NULL CR(0x0d),LF(0x0a),EOF(0xff)等。这样发生溢出的时候会被截断。
2. Random canaries:使用随机值代替固定值。防止攻击者还原现场。
3. Random XOR canaries: 在使用随机值的基础上增加一个与之XOR的控制数据。增加攻击难度。


示例见代码 stack_canaries/exam.c

```
 gcc -fstack-protector exam.c  -o f.out

// 这边程序终止并抛出错误 不开启 fno-srack-protector 则是段错误
 *** stack smashing detected ***: terminated
Aborted (core dumped)
```

补充个概念 TLS thread local storage 线程局部存储。用于为每个线程存储与其相关的数据的机制。每个线程都有其专用的TLS，数据在其中存储并且独立于其他线程。这样一来，同一个程序的不同线程可以同时访问相同的代码，但是每个线程都拥有自己独享的数据副本。当多个线程需要修改同一个变量的时候TLS会为每个线程提供一个变量的副本。

TCB thread control block

```C
typedef struct tcbhead_t {
    void *tcb;                // 指向线程控制块的指针   8
    ptrdiff_t dtv_slotinfo;   // 动态线程变量插槽信息   8
    void *self;               // 指向线程本身的指针     8
    int multiple_threads;     // 4                   
    int gscope_flag;          // 4                    
    uintptr_t sysinfo;        // 8                 
    uintptr_t stack_guard;
    // 其他与线程管理相关的字段
} tcbhead_t;
```


下面是gdb 反汇编main函数的结果
```
   0x0000000000001145 <+0>:     push   %rbp
   0x0000000000001146 <+1>:     mov    %rsp,%rbp
   0x0000000000001149 <+4>:     sub    $0x30,%rsp
   0x000000000000114d <+8>:     mov    %edi,-0x24(%rbp)
   0x0000000000001150 <+11>:    mov    %rsi,-0x30(%rbp)
   0x0000000000001154 <+15>:    mov    %fs:0x28,%rax
   0x000000000000115d <+24>:    mov    %rax,-0x8(%rbp)
   0x0000000000001161 <+28>:    xor    %eax,%eax
   0x0000000000001163 <+30>:    lea    -0x12(%rbp),%rax
   0x0000000000001167 <+34>:    mov    %rax,%rsi
   0x000000000000116a <+37>:    lea    0xe93(%rip),%rdi        # 0x2004
   0x0000000000001171 <+44>:    mov    $0x0,%eax
   0x0000000000001176 <+49>:    call   0x1040 <__isoc99_scanf@plt>
   0x000000000000117b <+54>:    nop
   0x000000000000117c <+55>:    mov    -0x8(%rbp),%rax
   0x0000000000001180 <+59>:    sub    %fs:0x28,%rax
   0x0000000000001189 <+68>:    je     0x1190 <main+75>
   0x000000000000118b <+70>:    call   0x1030 <__stack_chk_fail@plt>
   0x0000000008001190 <+75>:    leave  
   0x0000000008001191 <+76>:    ret
```

在linux在  fs寄存器用于存储tls.这边**mov %fs:0x28,%rax** 偏移0x28就是`stack_guard` 然后将其放到`mov    %rax,-0x8(%rbp)`rbp-8的位置。在函数返回之前将其取出和原来的值做sub操作。如果不相等则调用`<__stack_chk_fail@plt>`栈帧检测失败。

如果不适用tls存储canary 也可以存储再.bss段。基于上述说明，那么想要突破canary防护就两种思路，一个就是在栈溢出的时候同时将canary值还原，还有就是将存储在tls或者.bss中的canary同时修改。


下面有个例子 NJCTF 2017 messager.其中是使用爆破的方式逐字节猜测canary值。出去最后字节固定`\x00`作为截断作用.

前提 fork子进程的canary和父进程一致

还有就是之前学习汇编。rbp指针 base pointer.指向函数栈帧。可以用来访问当前函数栈帧中的局部变量和函数参数等。

这边举个没有canary的例子，见代码[code/stack_canaries/at_no_canary.py 以及 no_stack_canary.c]

```
0000000000401145 <vulnerable>:
  401145:       55                      push   %rbp
  401146:       48 89 e5                mov    %rsp,%rbp
  401149:       48 83 ec 10             sub    $0x10,%rsp
  40114d:       48 8d 3d b9 0e 00 00    lea    0xeb9(%rip),%rdi        # 40200d <_IO_stdin_used+0xd>
  401154:       e8 d7 fe ff ff          callq  401030 <puts@plt>
  401159:       48 8d 45 f4             lea    -0xc(%rbp),%rax
  40115d:       48 89 c7                mov    %rax,%rdi
  401160:       b8 00 00 00 00          mov    $0x0,%eax
  401165:       e8 d6 fe ff ff          callq  401040 <gets@plt>
  40116a:       48 8d 45 f4             lea    -0xc(%rbp),%rax
  40116e:       48 89 c7                mov    %rax,%rdi
  401171:       e8 ba fe ff ff          callq  401030 <puts@plt>
  401176:       90                      nop
  401177:       c9                      leaveq
  401178:       c3                      retq


0000000000401132 <success>:
  401132:       55                      push   %rbp
  401133:       48 89 e5                mov    %rsp,%rbp
  401136:       48 8d 3d c7 0e 00 00    lea    0xec7(%rip),%rdi        # 402004 <_IO_stdin_used+0x4>
  40113d:       e8 ee fe ff ff          callq  401030 <puts@plt>
  401142:       90                      nop
  401143:       5d                      pop    %rbp
  401144:       c3                      retq
```

这边是objdump -d出来发现success的地址在0x401132 然后s的基于rbp的偏移是0xc所以这边长度就是0xc加8byte的canary值，后面就是ret地址。

结果如下:

```
b'AAAAAAAAAAAAAAAAAAAA2\x11@\nsuccess!\n'
```


<div id=c1-3-2><h3>No eXecute</h3></div>

这是将内存页属性设置为不可知性从而阻止攻击。这边bit位可以参见`OS/blog_os/blog_os_cn/paging-introduction.md`。部分攻击会通过栈溢出的方式将恶意代码植入。这时候如果pc指向这些nx，会触发硬件层面的异常。需要硬件以及软件方面的支持。需要支持分页系统。然后`GOT`部分见`source2elf.md`这边代码一般.text才是可执行的。那么这些溢出方式攻击`GOT`的方式一般无效。但是代码重用来进行攻击 ret2libc 还是可用。因为本身`nx`bit是0.比如将代码返回地址变为libc的system函数然后传递参数设置为某条shell指令。这时候nx保护机制就无效了。

攻击有很多种方式 其中包含栈溢出覆盖返回地址 这种可以通过canary值来保护.某些情况下 rop攻击通过已有的函数指令来实现攻击，比如通过修改寄存器或者内存数据。程序中已经含有类似system等调用 将栈上或者寄存器种数据修改实现shellcode.此处并不需要修改函数返回地址



gcc -z是针对运行时栈的设置。


1. -z execstack: 这个选项告诉链接器生成的可执行文件或共享对象的栈是可执行栈。也就是说，栈空间可以包含可执行代码。
2. -z noexecstack: 这个选项与 -z execstack 相反，它使得生成的对象的栈空间不可执行，这是一个常用的安全特性，可以防止一些缓冲区溢出的攻击。
3. -z relro: 这个选项让链接器把进程启动后，不会再被修改的部分进行标记。启动这个选项有助于防止某些类型的缓冲区溢出攻击。
4. -z now: 当打开了这个选项，动态链接器会在程序启动时解析所有的动态符号，这样可以避免潜在的运行时解析的问题。
4. -z lazy: 这个选项与 -z now 相反，动态链接器会在程序运行过程中，延迟解析动态符号，直到这些符号被用到。


默认是rw不可知性。`readelf -l`可以看到。


下面是一个攻击示例,代码见`nx/code/dep.c`

`\x31\xc9\xf7\xe1\xb0\x0b\x51\x68\x2f\x2f\x73\x68\x68\x2f\x62\x69\x6e\x89\xe3\xcd\x80`

下面是对这个 shellcode 的简单解释：

- \x31\xc9：xorl %ecx,%ecx；将 ecx 寄存器清零。
- \xf7\xe1：pushl %ecx；将清零后的 ecx 寄存器的值压入栈中。
- \xb0\x0b：movb $0xb,%al；将值 0xb 赋给 al 寄存器，这是系统调用号，对应于 Linux 的 execve 系统调用。
- \x51：pushl %ecx；再次将 ecx 寄存器的值（0）压入栈中，作为 execve 的 argv 指针。
- \x68\x2f\x2f\x73\x68：pushl $0x68732f2f；将字符串 "//sh" 的反序形式压入栈中，作为 execve 的参数之一。
- \x68\x2f\x62\x69\x6e：pushl $0x6e69622f；将字符串 "/bin" 的反序形式压入栈中，与前面的 "//sh" 组合成 "/bin//sh"。
- \x89\xe3：movl %esp,%ebx；将栈指针 esp 的值赋给 ebx 寄存器，此时 ebx 指向刚才压入的 "/bin//sh" 字符串。
- \xcd\x80：int $0x80；触发中断 0x80，执行 Linux 系统调用。
当这个 shellcode 被插入到程序的某个缓冲区，并由于缓冲区溢出而被执行时，它会尝试启动一个新的 shell。

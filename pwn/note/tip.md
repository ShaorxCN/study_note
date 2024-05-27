call xxxx ；默认动作会将下条指令地址铺设

leave 会达成类似下面语句的效果

```asm
mov esp,rbp
pop rbp
```

ret 则是类似`pop eip`



具体结构还是需要参见具体汇编代码。

下面一个例子梳理下 之前一直先看书死记  


首先是x86的  32位依靠栈传递参数


```asm
gef➤  disassemble main
Dump of assembler code for function main:
   0x565561b5 <+0>:     push   ebp
   0x565561b6 <+1>:     mov    ebp,esp
   0x565561b8 <+3>:     call   0x565561df <__x86.get_pc_thunk.ax>
   0x565561bd <+8>:     add    eax,0x2e43
   0x565561c2 <+13>:    push   0x88
   0x565561c7 <+18>:    push   0x77
   0x565561c9 <+20>:    push   0x66
   0x565561cb <+22>:    push   0x55
   0x565561cd <+24>:    push   0x44
   0x565561cf <+26>:    push   0x33
   0x565561d1 <+28>:    push   0x22
   0x565561d3 <+30>:    push   0x11
   0x565561d5 <+32>:    call   0x56556189 <func>
   0x565561da <+37>:    add    esp,0x20
   0x565561dd <+40>:    leave  
   0x565561de <+41>:    ret    
End of assembler dump.
```

这边首先将8个参数右到左的顺序入栈 然后call(call 默认动作会将下调指令的地址入栈) 后面的add是恢复esp的 先略过 通过call 进入func部分 

这里有个`call   0x565561df <__x86.get_pc_thunk.ax> ` 是获取当前程序pc[这里是位置无关代码会出现这个操作]。 这个函数

```asm
gef➤  disassemble 0x565561df
Dump of assembler code for function __x86.get_pc_thunk.ax:
   0x565561df <+0>:     mov    eax,DWORD PTR [esp]
   0x565561e2 <+3>:     ret   
```

这样 `call 0x565561df` 本身会将下一条指令的指令入栈，也就是`0x565561bd` 然后ret后则是`add eax 0x2e43` 结果是 `0x56559000`

```asm
gef➤  reg
$eax   : 0x56559000  →  <_GLOBAL_OFFSET_TABLE_+0000> cld 
$ebx   : 0x0       
$ecx   : 0x2ae5a9e8
$edx   : 0xffffd104  →  0x00000000
```

这边通过断点运行查询结果一致。此处eax则是保存了GOT的地址 详细内容如下:

```asm
gef➤  disassemble 0x56559000
Dump of assembler code for function _GLOBAL_OFFSET_TABLE_:
   0x56559000:  cld    
   0x56559001:  add    BYTE PTR ds:[eax],al
   0x56559004:  sbb    cl,0xff
   0x56559007:  mul    eax
   0x56559009:  mov    dh,bh
   0x5655900b:  test   DWORD PTR [eax+0x2d],0xf7de
End of assembler dump.
```

具体作用未知 在main中未使用到。应该是方便got中定位吧。



继续到`func`部分：


```asm
gef➤  disassemble func
Dump of assembler code for function func:
   0x56556189 <+0>:     push   ebp                                   # 保存caller‘s ebp
   0x5655618a <+1>:     mov    ebp,esp                               # 设置当前ebp
   0x5655618c <+3>:     sub    esp,0x10                              # 开辟局部变量空间
   0x5655618f <+6>:     call   0x565561df <__x86.get_pc_thunk.ax>    # 获取程序pc
   0x56556194 <+11>:    add    eax,0x2e6c                            # 定位GOT
   0x56556199 <+16>:    mov    eax,DWORD PTR [ebp+0x8]               # 读取arg1
   0x5655619c <+19>:    add    eax,0x1                               # 执行arg1+1
   0x5655619f <+22>:    mov    DWORD PTR [ebp-0x4],eax               # 计算结果赋值给loc1
   0x565561a2 <+25>:    mov    eax,DWORD PTR [ebp+0x24]              # 获取arg8
   0x565561a5 <+28>:    add    eax,0x8                               # 计算arg8+8
   0x565561a8 <+31>:    mov    DWORD PTR [ebp-0x8],eax               # 计算结果赋值给loc2
   0x565561ab <+34>:    mov    edx,DWORD PTR [ebp-0x4]               # loc1-->edx
   0x565561ae <+37>:    mov    eax,DWORD PTR [ebp-0x8]               # loc2-->eax
   0x565561b1 <+40>:    add    eax,edx                               # loc1+loc2 保存到eax 函数返回结果一般保存在eax寄存器
   0x565561b3 <+42>:    leave  
   0x565561b4 <+43>:    ret 
```

进入func后第一件事情是将caller的ebp入栈保存(main开始也是同理) 然后将当前栈顶作为callee的栈基址。 sub esp,0x10 就是两个32bit int就是开启局部便来那个 loc1 loc2的空间。
同样定位了GOT的位置。然后是一条指令将arg1参数保存到eax 验证如下

`mov    eax,DWORD PTR [ebp+0x8]）`

此处寄存器数据如下:

```
gef➤  reg
$eax   : 0x56559000  →  <_GLOBAL_OFFSET_TABLE_+0000> cld 
$ebx   : 0x0       
$ecx   : 0x462c4092
$edx   : 0xffffd104  →  0x00000000
$esp   : 0xffffd090  →  0xf7fac3fc  →  0xf7fad2c0  →  0x00000000
$ebp   : 0xffffd0a0  →  0xffffd0c8  →  0x00000000
$esi   : 0xf7fac000  →  0x001e3d6c
$edi   : 0xf7fac000  →  0x001e3d6c
$eip   : 0x56556199  →  <func+0010> mov eax, DWORD PTR [ebp+0x8]
$eflags: [zero carry PARITY ADJUST sign trap INTERRUPT direction overflow resume virtualx86 identification]
$cs: 0x23 $ss: 0x2b $ds: 0x2b $es: 0x2b $fs: 0x00 $gs: 0x63 
gef➤  p  $ebp+0x8
$4 = (void *) 0xffffd0a8
```

栈数据如下:

```
0xffffd090│+0x0000: 0xf7fac3fc  →  0xf7fad2c0  →  0x00000000     ← $esp
0xffffd094│+0x0004: 0x00000001
0xffffd098│+0x0008: 0x56559000  →  <_GLOBAL_OFFSET_TABLE_+0000> cld 
0xffffd09c│+0x000c: 0x5655623b  →  <__libc_csu_init+004b> add esi, 0x1
0xffffd0a0│+0x0010: 0xffffd0c8  →  0x00000000    ← $ebp
0xffffd0a4│+0x0014: 0x565561da  →  <main+0025> add esp, 0x20
0xffffd0a8│+0x0018: 0x00000011
0xffffd0ac│+0x001c: 0x00000022 ("""?)
```

```
gef➤  p &arg1
$7 = (int *) 0xffffd0a8
```

结果一致

后面的流程类似 参见代码注释  验证部分如下:

```
gef➤  p $ebp-0x4
$10 = (void *) 0xffffd09c
gef➤  p &loc1
$11 = (int *) 0xffffd09c
gef➤  p &loc2
$12 = (int *) 0xffffd098
gef➤  p &arg8
$13 = (int *) 0xffffd0c4
gef➤  p $ebp+0x24
$14 = (void *) 0xffffd0c4
gef➤  p &arg8
$15 = (int *) 0xffffd0c4
gef➤  p &loc2
$16 = (int *) 0xffffd098
gef➤  p $ebp-0x8
$17 = (void *) 0xffffd098
```



这是x86-64的  64位首先依靠rdi rsi rdx rcx r8 r9 传递参数 不足的通过栈
```asm
gef➤  disassemble main
Dump of assembler code for function main:
   0x0000000000001159 <+0>:     push   rbp
   0x000000000000115a <+1>:     mov    rbp,rsp
   0x000000000000115d <+4>:     push   0x88
   0x0000000000001162 <+9>:     push   0x77
   0x0000000000001164 <+11>:    mov    r9d,0x66
   0x000000000000116a <+17>:    mov    r8d,0x55
   0x0000000000001170 <+23>:    mov    ecx,0x44
   0x0000000000001175 <+28>:    mov    edx,0x33
   0x000000000000117a <+33>:    mov    esi,0x22
   0x000000000000117f <+38>:    mov    edi,0x11
   0x0000000000001184 <+43>:    call   0x1125 <func>
   0x0000000000001189 <+48>:    add    rsp,0x10
   0x000000000000118d <+52>:    leave  
   0x000000000000118e <+53>:    ret    
End of assembler dump.
gef➤  disassemble func
Dump of assembler code for function func:
   0x0000000000001125 <+0>:     push   rbp
   0x0000000000001126 <+1>:     mov    rbp,rsp
   0x0000000000001129 <+4>:     mov    DWORD PTR [rbp-0x14],edi
   0x000000000000112c <+7>:     mov    DWORD PTR [rbp-0x18],esi
   0x000000000000112f <+10>:    mov    DWORD PTR [rbp-0x1c],edx
   0x0000000000001132 <+13>:    mov    DWORD PTR [rbp-0x20],ecx
   0x0000000000001135 <+16>:    mov    DWORD PTR [rbp-0x24],r8d
   0x0000000000001139 <+20>:    mov    DWORD PTR [rbp-0x28],r9d
   0x000000000000113d <+24>:    mov    eax,DWORD PTR [rbp-0x14]
   0x0000000000001140 <+27>:    add    eax,0x1
   0x0000000000001143 <+30>:    mov    DWORD PTR [rbp-0x4],eax
   0x0000000000001146 <+33>:    mov    eax,DWORD PTR [rbp+0x18]
   0x0000000000001149 <+36>:    add    eax,0x8
   0x000000000000114c <+39>:    mov    DWORD PTR [rbp-0x8],eax
   0x000000000000114f <+42>:    mov    edx,DWORD PTR [rbp-0x4]
   0x0000000000001152 <+45>:    mov    eax,DWORD PTR [rbp-0x8]
   0x0000000000001155 <+48>:    add    eax,edx
   0x0000000000001157 <+50>:    pop    rbp
   0x0000000000001158 <+51>:    ret 

```
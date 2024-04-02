section .text
    global _start

_start:
    ; 调用 execve 系统调用执行 /bin/sh 命令
    mov rax, 59             ; 系统调用号：59 表示 execve
    mov rdi, sh_str         ; /bin/sh 字符串地址
    xor rsi, rsi            ; argv 参数为空，RSI=0
    xor rdx, rdx            ; envp 参数为空，RDX=0
    syscall

    ; 退出程序
    mov rax, 60             ; 系统调用号：60 表示 exit
    xor rdi, rdi            ; 传递退出码：0 表示成功
    syscall

section .data
    sh_str db '/bin/sh', 0   ; /bin/sh 字符串

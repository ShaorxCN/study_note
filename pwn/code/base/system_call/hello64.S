.data
msg:
    .ascii "hello 64-bit!\n"
    len = . - msg
.text
    .global _start
_start:
    movq $1,%rdi            # 文件描述符 输出目标
    movq $msg,%rsi          # 缓冲区指针
    movq $len,%rdx          # 字节数
    movq $1,%rax            # sys_write
    syscall
    
    xorq %rdi,%rdi         #  返回状态码 这里是0
    movq $60,%rax          #  sys_exit
    syscall
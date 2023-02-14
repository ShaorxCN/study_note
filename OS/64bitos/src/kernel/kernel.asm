[section .text]
global _start

_start:
    mov ax,0b800h
    mov gs,ax
    mov ah,0Fh           ;0：黑底 F:白字
    mov al,'K'
    mov [gs:((80*1+39)*2)],ax      ;第一行 低39列
    jmp $
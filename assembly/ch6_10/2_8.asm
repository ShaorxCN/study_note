;将每个单词改为大写
assume cs:codesg,ds:datasg,ss:stacksg
datasg segment
           db 'ibm             '    ;空格填充 16bytes  理论上4x3的循环次数
           db 'dec             '
           db 'dos             '
           db 'vax             '
    ; dw 0                     ;这边双循环 这里保存一个cx计数在内存中 40h 改成栈的版本
datasg ends

stacksg segment
            dw 0,0,0,0,0,0,0,0    ; 预先开辟16bytes的空间做栈
stacksg ends

codesg segment
    start: mov  ax,datasg
           mov  ds,ax
           mov  bx,0
           mov  ax, stacksg
           mov  ss,ax
           mov  sp,10h
           mov  cx,4            ;外层循环次数
    s0:    push cx
           mov  si,0
           mov  cx,3
    s:     mov  al,[bx+si]
           and  al,11011111b
           mov  [bx+si],al
           inc  si
           loop s

           add  bx,16
           pop  cx
           loop s0

           mov  ax,4c00h
           int  21h
codesg ends

end start
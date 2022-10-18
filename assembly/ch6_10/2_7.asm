;通过[bx+idata]的形式处理数组  第一个字符串转换为大写 第二个转换为小写
assume cs:code,ds:data
data segment
         db 'BaSiC'
         db 'Minix'
data ends

code segment
    start:mov  ax,data
          mov  ds,ax
          mov  bx,0
          mov  cx,5
    s:    mov  al,[bx]
          and  al,11011111b
          mov  [bx],al
          mov  al,5[bx]        ;5[bx]等价于[5+bx]
          or   al,00100000b
          mov  5[bx],al
          inc  bx
          loop s
        
          mov  ax, 4c00h
          int  21h
code ends

end start
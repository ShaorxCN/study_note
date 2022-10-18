; 在2000h段中寻找第一个位0的字节 将他的偏移位置存放到dx中
assume cs:codesg
codesg segment
    start: mov  ax,2000h
           mov  ds,ax
           mov  bx,0

    s:     mov  cl,[bx]
           mov  ch,0
           jcxz ok
           inc  bx
           jmp  short s
    ok:    mov  dx,bx
           mov  ax,4c00h
           int  21h
codesg ends
end start
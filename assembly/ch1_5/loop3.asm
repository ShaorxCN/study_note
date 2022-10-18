assume cs:code
code segment
         mov  ax,0ffffh    ;不能字母开头 所以是0ffffh
         mov  ds,ax
         mov  bx,6
         mov  cx,3
         mov  dx,0
         mov  al,[bx]
         mov  ah,0
    s:   add  dx,ax
         loop s
         mov  ax,4c00h
         int  21h
code ends
end
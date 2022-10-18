; 0ffff:0 b中数据累加到dx
assume cs:code
code segment
         mov  ax,0ffffh
         mov  ds,ax
         mov  ax,0
         mov  dx,0
         mov  bx,0
         mov  cx,12
    s:   mov  al,[bx]
         add  dx,ax
         inc  bx
         loop s

         mov  ax,4c00h
         int  21h
code ends
end
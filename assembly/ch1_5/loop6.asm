; 将ffff:0 b的数据复制到0:200 20b
assume cs:code
code segment
         mov  cx,12
         mov  bx,0

         mov  ax,0ffffh
         mov  ds,ax

         mov  ax,0020h
         mov  es,ax

    s:   mov  dl,[bx]
         mov  es:[bx],dl
         inc  bx
         loop s
         mov  ax,4c00h
         int  21h
code ends
end
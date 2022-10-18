; 将ffff:0 b的数据复制到0:200 20b
assume cs:code
code segment
         mov  cx,12
         mov  bx,0
    s:   mov  ax,0ffffh
         mov  ds,ax
         mov  dl,[bx]
         mov  ax,0020h     ; 0:200 就是00200等价于 0020:0 也是00200
         mov  ds,ax
         mov  [bx],dl
         inc  bx
         loop s
         mov  ax,4c00h
         int  21h
code ends
end
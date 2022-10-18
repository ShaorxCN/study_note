; 将代码mov ax,4c00h之前本身复制到0:200处

assume cs:code
code segment
         mov  ax,cs
         mov  ds,ax
         mov  cx,17h
         mov  ax,0020h
         mov  es,ax
         mov  bx,0
    s:   mov  al,[bx]
         mov  es:[bx],al
         inc  bx
         loop s
         mov  ax,4c00h
         int  21h
code ends
end

assume cs:code,ds:data,ss:stack
data segment
         dw 0123h,0456h,0789h,0abch,0defh,0fedh,0cbah,0987h
data ends

stack segment
          dw 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
stack ends

code segment                ;这里因为将dw独立出去了 code和cs关联 里面直接就是代码 实际这里ip就还是0了
    start:mov  ax,data
          mov  ds,ax
          mov  ax,stack
          mov  ss,ax
          mov  cx,8
          mov  bx,0
          mov  sp,20h
         
    s:    push [bx]
          add  bx,2
          loop s

          mov  bx,0
          mov  cx,8
    s0:   pop  [bx]
          add  bx,2
          loop s0
          mov  ax, 4c00h
          int  21h
code ends
end start
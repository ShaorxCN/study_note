; 将给出的数据逆序存放
assume cs:code
code segment
          dw   0123h,0456h,0789h,0abch,0defh,0fedh,0cbah,0987h
          dw   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0                    ; 准备空间当作栈使用
    start:mov  ax,cs
          mov  ss,ax                                              ;2f+1 两个dw声明是2f f+1f
          mov  sp,30h
          mov  bx,0
          mov  cx,8
    s:    push cs:[bx]                                            ; dw在最开始声明  所以偏移在最开始
          add  bx,2
          loop s

          mov  bx,0
          mov  cx,8
    s0:   pop  cs:[bx]
          add  bx,2
          loop s0

          mov  ax,4c00h
          int  21h
code ends
end start ; end start 表示是从start开始 就直接从mov开始 程序入口,ip默认就不是0了 
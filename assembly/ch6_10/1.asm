; 将给出的数据累加 结果放入ax中
assume cs:code
code segment
           dw   0123h,0456h,0789h,0abch,0defh,0fedh,0cbah,0987h     ; dw 申明字型数据 这里是16b 但是这样一开始的指令是一些处理数据的
     start:mov  bx,0
           mov  ax,0
           mov  cx,8
     s:    add  ax,cs:[bx]                                          ; dw在最开始声明  所以偏移在最开始
           add  bx,2
           loop s
           mov  ax,4c00h
           int  21h
code ends
end start ; end start 表示是从start开始 就直接从mov开始 程序入口,ip默认就不是0了 
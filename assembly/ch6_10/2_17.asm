;计算data中第一组数据的三次方 存放到后面一组dword数据中
assume cs:codesg,ds:datasg
datasg segment
           dd 1,2,3,4,5,6,7,8
           dw 0,0,0,0,0,0,0,0
datasg ends
codesg segment
    start: mov  ax,datasg
           mov  ds,ax

           mov  si,0
           mov  di,16
           mov  cx,8
    s:     mov  bx,[si]
           call cube
           mov  [di],ax
           mov  [di].2,dx
           add  si,2
           add  di,4
           loop s
           mov  ax,4c00h
           int  21h

    cube:  mov  ax,bx
           mul  bx
           mul  bx
           ret
codesg ends
end start




;在屏幕中间分别显示 绿色 绿底红色 白底蓝色的 hello,evan(三行)
assume cs:codesg,ds:datasg,ss:stacksg
datasg segment
           db 'hello,evan'
           db 02h,24h,71h
datasg ends
stacksg segment
            dw 0,0,0,0,0,0,0,0
stacksg ends
codesg segment
    start: mov  ax,datasg
           mov  es,ax
           mov  ax,0b800h
           mov  ds,ax
           mov  ax,stacksg
           mov  ss,ax
           mov  sp,10h
           mov  bx,0
           mov  cx,3
           mov  bp,10
           mov  bx,780h          ;一共 25行 一行160字节 那么中间就是12 13 14行 就是780h
    s:     push cx
           mov  cx,10
           mov  di,0
           mov  si,0
           mov  ah,es:[bp]
          
    s0:    mov  al,es:[di]
           mov  [bx+64+si],al
           mov  [bx+65+si],ah
           add  si,2
           inc  di
           loop s0
           add  bx,160
           pop  cx
           add  bp,1
           loop s

           mov  ax,4c00h
           int  21h
codesg ends
end start
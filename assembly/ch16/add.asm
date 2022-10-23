assume cs:codesg,ds:datasg
datasg segment
    a      db 1,2,3,4,5,6,7,8
    b      dw 0
datasg ends

codesg segment
    start: mov  ax,datasg
           mov  ds,ax
           mov  si,0
           mov  cx,8
    s:     mov  al,a[si]     ;这里一定要assume ds:datasg 不然编译器不知道a的段地址在哪儿 当然上面的ds赋值也是必须的 assume只是关联
           mov  ah,0
           add  b,ax
           inc  si
           loop s
           mov  ax,4c00h
           int  21h
codesg ends
end start
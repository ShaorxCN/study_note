;将data中的数据全部转换为大写
assume cs:codesg,ds:datasg,ss:stacksg
datasg segment
           db 'word',0
           db 'unix',0
           db 'wind',0
           db 'good',0
datasg ends
stacksg segment
            db 8 dup(0)
stacksg ends
codesg segment
    start: 
           mov  ax,datasg
           mov  ds,ax
           mov  ax,stacksg
           mov  ss,ax
           mov  sp, 10h
           mov  bx,0
           mov  cx,4
    s:     mov  si,bx
           call upper
           add  bx,5
           loop s
           mov  ax,4c00h
           int  21h
    upper: push cx
    change:mov  cl,[si]
           mov  ch,0
           jcxz ok
           and  byte ptr [si],11011111b
           inc  si
           jmp  short change
    ok:    pop  cx
           ret
codesg ends

end start
;将某个字符串都变为大写
assume cs:codesg,ds:datasg
datasg segment
           db 'conversation'
datasg ends
codesg segment
    start: mov  ax,datasg
           mov  ds,ax
           mov  cx,12
           mov  bx,ax
           mov  si,0
           call upper
           mov  ax,4c00h
           int  21h
    upper: and  byte ptr [si],11011111b
           inc  si
           loop upper
           ret

codesg ends
end start
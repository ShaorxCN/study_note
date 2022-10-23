;将一个全是字母 以0几位的字符串转化为大写 
;ds:si指向首地址
;结果 没有返回值
assume cs:codesg,ds:datasg 
datasg segment
           db 'conversation',0
datasg ends
codesg segment
    start: mov  ax,datasg
           mov  ds,ax
           mov  si,0
           mov  ch,0
    s:     mov  cl,[bx].[si]
           jcxz ok
           and  cl,11011111b    ;可以不通过cl 直接 add byte ptr [si],11011111b
           mov  [bx].[si],cl
           inc  si
           jmp  short s
    ok:    mov  ax, 4c00h
           int  21h
codesg ends

end start
;计算 100001/100 因为100001 大于65535 所以是32位被除数
assume cs:codesg,ss:stacksg
stacksg segment
            dw 0,0,0,0,0,0,0,0
stacksg ends
codesg segment
    start: mov  ax,stacksg
           mov  ss,ax
           mov  sp,10h
           mov  dx,1
           mov  ax,86a1h

           mov  bx,100
           div  bx

           push ax
           push dx
           mov  ax,4c00h
           int  21h
codesg ends
end start



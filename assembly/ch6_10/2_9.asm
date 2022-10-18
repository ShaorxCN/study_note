;将datasg段中每个单词的前四个字母改为大写字母
assume cs:codesg,ds:datasg,ss:stacksg

stacksg segment
            dw 0,0,0,0,0,0,0,0
stacksg ends

datasg segment
           db '1. display      '
           db '2. brows        '
           db '3. repalce      '
           db '4. modify       '
datasg ends

codesg segment
    start: mov  ax,datasg
           mov  ds,ax
           mov  ax,stacksg
           mov  ss,ax
           mov  sp,10h
           mov  cx,4
           
           mov  bx,0

    s0:    push cx
           mov  si,3            ;第三个开始转换
           mov  cx,4
    s:     mov  al,[bx+si]
           and  al,11011111b
           mov  [bx+si],al
           inc  si
           loop s

           pop  cx
           add  bx,16
           loop s0
    
           mov  ax,4c00h
           int  21h
codesg ends
end start
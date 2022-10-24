;使用16h中断读取一个字符 输入r字符 将屏幕上的字符变为红色 g 绿色 b 蓝色
assume cs:codesg
codesg segment
    start: 
           mov  ah,0
           int  16h
           mov  ah,1                          ;rgb 预设蓝色 左移一位时绿色 两位时红色
           cmp  al,'r'
           je   red
           cmp  al,'g'
           je   green
           cmp  al,'b'
           je   blue
           jmp  short sret
    red:   shl  ah,1                          ;这里移位时绿色 但是没有跳转所以接着执行到green再次移动一位 变为红色
    green: shl  ah,1
    blue:  mov  bx,0b800h
           mov  es,bx
           mov  bx,1
           mov  cx,2000
    s:     and  byte ptr es:[bx],11111000b
           or   es:[bx],ah
           add  bx,2
           loop s
    sret:  mov  ax,4c00h
           int  21h
codesg ends
end start
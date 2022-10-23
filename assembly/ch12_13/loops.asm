;7ch中断例程模拟loop s指令
;在屏幕中间显示10个"!"
assume cs:codesg
codesg segment
    start: mov  ax,0                         ;安装部分
           mov  es,ax
           mov  di,200h
           mov  ax,cs
           mov  ds,ax
           mov  si,offset lp
           mov  cx,offset lpend-offset lp
           cld
           rep  movsb
           mov  ax,0                         ; 设置向量表  8086向量表默认是在0000开始的1024字节
           mov  es,ax
           mov  word ptr es:[7ch*4],200h
           mov  word ptr es:[7ch*4+2],0      ;设置结束

           mov  ax,0b800h
           mov  es,ax
           mov  di,12*160
           mov  bx,offset s-offset se        ;获取se到s的偏移值
           mov  cx,10
    s:     mov  byte ptr es:[di],'!'
           mov  byte ptr es:[di+1],4
           add  di,2
           int  7ch
    se:    nop
           mov  ax,4c00h
           int  21h
    lp:    push bp                           ;保留bp值
           mov  bp,sp                        ;获取当前栈顶 最上面放的是ip偏移 应该是int 7ch后的也就是se
           dec  cx                           ; cx-1
           jcxz lpret                        ;0就返回
           add  [bp+2],bx                    ;获取 se加上s到se的偏移值 也就是说到s的位置 然后pop bp iret返回的是处理过的sp
    lpret: pop  bp
           iret
    lpend: nop
codesg ends
end start
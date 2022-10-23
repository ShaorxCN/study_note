;重写除法溢出中断 0的中断程序
;start中安装do0 然后设置中断向量表
;movsb: es:di = ds:si
assume cs:codesg
codesg segment
    start:   mov  ax,0                           ;开始安装do0
             mov  es,ax
             mov  di,200h                        ;假设这边向量表位置是空的 可以存放程序do0
             mov  ax,cs
             mov  ds,ax
             mov  si,offset do0
             mov  cx,offset do0end-offset do0
             cld
             rep  movsb
             mov  ax,0                           ; 设置向量表  8086向量表默认是在0000开始的1024字节
             mov  es,ax
             mov  word ptr es:[0*4],200h
             mov  word ptr es:[0*4+2],0
             mov  ax,4c00h
             int  0
    do0:     jmp  short do0start
             db   "evan overflow!"               ;保存在do0自己里面 这样保证overflow!本身的存储 不会擦除 产生错误
    do0start:mov  ax,cs
             mov  ds,ax
             mov  si,202h                        ;jmp双字节指令

             mov  ax,0b800h
             mov  es,ax
             mov  di,12*160+36*2                 ;指向中间位置
             mov  cx,14
             mov  ah, 4
    s:       mov  al,[si]
             mov  word ptr es:[di],ax
             inc  si
             add  di,2
             loop s
         
             mov  ax,4c00h
             int  21h
    do0end:  nop
codesg ends
end start
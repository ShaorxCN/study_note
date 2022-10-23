;从CMOSRAM中读取月份并且显示在屏幕中间
;BCD码是10进制的2进制存储 那么+30h就是十进制对应的ascii值
;比如1是49 1+30h(49) = 1 因为0的ascii刚好是00110000 前四位刚好48 然后递增就是
assume cs:codesg
codesg segment
    start: mov al,8
           out 70h,al
           in  al,71h                            ;获取月份的bcd码存入al

           mov ah,al
           mov cl,4
           shr ah,cl                             ;右移四位刚好是月份的十位数字
           and al,00001111b                      ;保留个位的数字

           add ah,30h
           add al,30h

           mov bx,0b800h
           mov es,bx
           mov byte ptr es:[160*12+40*2],ah
           mov byte ptr es:[160*12+40*2+1],4

           mov byte ptr es:[160*12+40*2+2],al
           mov byte ptr es:[160*12+40*2+3],4

           mov ax,4c00h
           int 21h
codesg ends
end start
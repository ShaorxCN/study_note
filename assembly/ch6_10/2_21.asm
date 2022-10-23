;在屏幕指定位置显示字符串 无溢出除法 转换为十进制三个小程序
;show_str: dh 行号 dl 列号 cl 颜色  ds:si指向首地址
;被除数x 除数n  h x高16位 l x低十六位  int() 取商 rem()取余数  x/n=int(h/n)*65536+[rem(h/n)*65536+l]/n(前半部分就是结果的高位 后面是低位和余数)
; ax=dworld 低16位 dx=dword高16位 cx=除数  返回 dx=结果高16位 ax 结果低16位 cx 余数 其实是拆成两个32位除法
assume cs:codesg,ds:datasg,ss:stacksg
datasg segment
           db 'welcome to masm',0
datasg ends
stacksg segment
            db 8 dup(0)
stacksg ends
codesg segment
    start:   mov  ax,datasg      ;在屏幕第八行 第三列 绿色展示
             mov  ds,ax
             mov  ax,stacksg
             mov  ss,ax
             mov  sp,10h
             mov  si,0

             mov  dh,8
             mov  dl,3
             mov  cl,02h
             mov  si,0

             call show_str

             mov  ax,4240h
             mov  dx,000fh
             mov  cx,0ah
             call divdw
    
             mov  ax,4c00h
             int  21h
    
    show_str:mov  al,dh          ;屏幕指定位置显示字符串
             mov  ah,0
             mov  bx,160
             mov  ch,0
             push dx             ;乘法结果大于8位 高位放在dx 低位放在ax 但是结果肯定是16放的下的
             mul  bx
             pop  dx             ;还原颜色
             mov  di,ax          ;di是开始要写入的行地址
             mov  bh,0
             mov  bl,dl          ;列号写入 bx 然后默认都是正数 第一列实际是0开始
             dec  bx
             add  bx,bx
             add  di,bx
             mov  ax,0b800h
             mov  es,ax
             push cx
             mov  ah,cl          ;固定颜色在高位
             mov  ch,0
    write:   mov  al,[si]
             mov  cl,al
             jcxz ok
             mov  es:[di],ax
             inc  si
             add  di,2
             jmp  short write
    ok:      pop  cx
             ret
    divdw:   push ax             ; 无溢出除法 存储低16位
             mov  ax,dx          ; 准备做高16位部分 dx 和 ax都是高位
             mov  dx,0           ; 这里做32位的除法部分
             div  cx             ; ax中放商部分 dx中余数
             mov  bx,ax          ; 保存int(h/n)
             pop  ax             ; ax中是低位 现在dx中已经是rem(h/n)作为高位
             div  cx             ; ax中是[rem(h/n)*65536+l]/n的商 dx中是[rem(h/n)*65536+l]/n的余数
             mov  cx,dx          ; 纪录余数
             mov  dx,bx          ; 存储 int(h/n)且作为高位
             ret
codesg ends

end start
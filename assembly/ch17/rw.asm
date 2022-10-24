;编写一个程序 实现读取键盘输入字符串并且显示在屏幕上
;回车结束输入
;dh,dl表示显示的行列位置  ds:si指向字符串的存储空间  0结尾 可以删除
assume cs:codesg
codesg segment
    start:    call getstr
              mov  ax,4c00h
              int  21h
    getstr:   push ax
    getstrs:  mov  ah,0
              int  16h
              cmp  al,20h
              jb   nochar                       ;小于20h说明不是字符
              mov  ah,0
              call charstack
              mov  ah,2
              call charstack
              jmp  getstrs
    nochar:   cmp  ah,0eh                       ;退格键
              je   backspace
              cmp  ah,1ch                       ;enter
              je   enterkey
              jmp  getstrs
    backspace:mov  ah,1
              call charstack
              mov  ah,2
              call charstack
              jmp  getstrs
    enterkey: mov  al,0
              mov  ah,0
              call charstack
              mov  ah,2
              call charstack
              pop  ax
              ret
    
    ;新的子程序 ah为功能号 0表示al=入栈字符 1 删除一个字符 al=返回的字符 2:展示字符 dh,dl行列号
    charstack:jmp  short charstart
    menu      dw   charpush,charpop,charshow
    top       dw   0                            ;栈顶
    charstart:push bx
              push dx
              push di
              push es
              cmp  ah,2
              ja   sret
              mov  bl,ah
              mov  bh,0
              add  bx,bx
              jmp  word ptr menu[bx]
    charpush: mov  bx,top
              mov  [si][bx],al
              inc  top
              jmp  sret
    charpop:  cmp  top,0
              je   sret
              dec  top
              mov  bx,top
              mov  al,[si][bx]
              jmp  sret
    charshow: mov  bx,0b800h
              mov  es,bx
              mov  al,160
              mov  ah,0
              mul  dh
              mov  di,ax
              add  dl,dl
              mov  dh,0
              add  di,dx
              mov  bx,0
    charshows:cmp  bx,top
              jne  noempty
              mov  byte ptr es:[di],' '
              jmp  sret
    noempty:  mov  al,[si][bx]
              mov  es:[di],al
              mov  byte ptr es:[di+2],' '
              inc  bx
              add  di,2
              jmp  charshows
    sret:     pop  es
              pop  di
              pop  dx
              pop  bx
              ret
   
codesg ends
end start
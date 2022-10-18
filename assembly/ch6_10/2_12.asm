;实验7
assume cs:codesg,ds:datasg,es:tablesg,ss:stacksg
datasg segment
              db '1975','1976','1977','1978','1979','1980','1981','1982','1983'
              db '1984','1985','1986','1987','1988','1989','1990','1991','1992'
              db '1993','1994','1995'
       ;以上是表示 21 年的字符串 4 * 21 = 84
              dd 16,22,382,1356,2390,8000,16000,24486,50065,97479,140417,197514
              dd 345980,590827,803530,1183000,1843000,2759000,3753000,4649000,5937000
       ;以上是表示 21 年公司总收入的 dword 型数据 4 * 21 = 84
              dw 3,7,9,13,28,38,130,220,476,778,1001,1442,2258,2793,4037,5635,8226
              dw 11542,14430,15257,17800
       ;以上是表示 21 年公司雇员人数的 21 个 word 型数据 2 * 21 = 42
datasg ends
tablesg segment
               db 21 dup ('year summ ne ?? ')       ; 'year summ ne ?? ' 刚好 16 个字节
tablesg ends
stacksg segment
               db 8 dup (0)
stacksg ends
codesg segment
       start: mov  ax,datasg
              mov  ds,ax
              mov  ax,tablesg
              mov  es,ax
              mov  ax,stacksg
              mov  ss,ax
              mov  sp,10h

              mov  cx,21                      ; 21此循环 21年
              mov  bx,0
              mov  di,0                       ; 年份数据地址
              mov  bp,0

       s0:    push cx
              mov  si,0
              mov  cx,4                       ; 收入和雇员都是4bytes

       s:     mov  al,[di]                    ;处理年份和收入
              mov  es:[bx+si],al
              
              mov  al,[di+54h]
              mov  es:[bx+5+si],al
              inc  di
              inc  si
              loop s
              
              mov  cx,2
       s1:    mov  al,[bx+0a8h]               ;处理雇员
              mov  es:[bx+6+si],al
              inc  si
              loop s1
              mov  dx,es:[bx+7h]              ;处理人均
              mov  ax,es:[bx+5h]

              div  word ptr es:[bx+0ah]
              mov  es:[bx+0dh],ax
              add  bx,10h
              pop  cx
              loop s0

              mov  ax,4c00h
              int  21h

codesg ends

end start
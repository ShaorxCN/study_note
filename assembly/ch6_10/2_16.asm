assume cs:codesg,ds:datasg,ss:stacksg
datasg segment
              db 'welcome to masm!'
              db 02h,24h,71h              ;三种颜色
datasg ends
stacksg segment
               db 16 dup (0)
stacksg ends
codesg segment
       ;datasg 段地址
       start: mov  ax,datasg
              mov  ds,ax
 
       ;stacksg 段地址
              mov  ax, stacksg
              mov  ss,ax
              mov  sp,10h
 
       ;目标地址
              mov  ax,0b800h
              mov  es,ax
 
              mov  cx,3
              mov  di,10h                   ;在 datasg 中的偏移量
              mov  bx,780h                  ;表示第 12 行
       ;第一层循环，3 种颜色
       s:     mov  si,0                     ;在显示缓冲区中的偏移地址
              mov  ah,ds:[di]
              push cx
              push di
 
              mov  di,0
              mov  cx,16
       ;第二层循环，字符串
       s0:    mov  al,ds:[di]
              mov  es:[bx+si+64],al         ;低位存字符
              mov  es:[bx+si+64+1],ah       ;高位存属性
              inc  di
              add  si,2
              loop s0
 
              pop  di
              pop  cx
              inc  di
              add  bx,0a0h
              loop s
 
              mov  ax,4c00h
              int  21h
codesg ends
end start

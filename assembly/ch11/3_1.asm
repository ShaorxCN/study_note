;将包含任意字符 以0结尾的字符串中的小写字母转换为大写字母
; ds:si指向字符串的首字母
assume cs:codesg,ds:datasg
datasg segment
           db "Beginner's All-purpose Symbolic Instruction Code.",0
datasg ends
codesg segment
    start:  
            mov  ax,datasg
            mov  ds,ax
            mov  si,0
            call letterc
            mov  ax,4c00h
            int  21h
    letterc:cmp  byte ptr [si],97
            jb   next
            cmp  byte ptr [si], 122
            ja   next
            and  byte ptr [si],11011111b
    next:   mov  cl,[si]
            mov  ch,0
            jcxz ok
            inc  si
            jmp  short letterc
    ok:     ret
            
codesg ends
end start
;通过x ptr指定数据长度 某数据段如下 修改 137->30 40->110 'PDP'->'VAX'
assume cs:codesg,ds:datasg

datasg segment
           db 'DEC'
           db 'Ken Oslen'
           dw 137,40
           db 'PDP'
datasg ends

codesg segment
    start: mov ax,datasg
           mov ds,ax
           mov bx,0
           mov word ptr [bx].0ch,30         ; 必须指定 不然 operand must have size
           add word ptr [bx].0eh,70
           mov si,0
           mov byte ptr [bx].10h[si],'V'    ; 等价于[bx+idata+si]
           inc si
           mov byte ptr [bx].10h[si],'A'
           inc si
           mov byte ptr [bx].10h[si],'X'

           mov ax,4c00h
           int 21h
codesg ends

end start
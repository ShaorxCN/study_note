;用10h中断例程 实现5行12列处显示三个红底高亮闪烁绿色的'a'
assume cs:codesg
codesg segment
    start: mov ah,2            ;代表调用光标子程序
           mov bh,0            ;页码
           mov dh,5            ;行号
           mov dl,12           ;列号
           int 10h

           mov ah,9            ;光标处显示字符子程序
           mov al,'a'          ;字符
           mov bl,11001010b    ;颜色属性
           mov bh,0            ;页数
           mov cx,3            ;字符重复次数
           int 10h

           mov ax,4c00h
           int 21h
codesg ends
end start
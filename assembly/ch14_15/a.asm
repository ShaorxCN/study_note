;重写安装int9 当按下a时 除非不松开 否则全屏9 其他照旧
assume cs:codesg,ss:stacksg
stacksg segment
            db 128 dup (0)
stacksg ends
codesg segment
    start:  
            mov   ax,stacksg
            mov   ss,ax
            mov   sp,128
    
            push  cs                               ;将中断例程写到 0:204h
            pop   ds
            
 
            mov   ax,0
            mov   es,ax

 
            mov   cx,offset int9end-offset int9
            mov   si,offset int9
            mov   di,204h
            cld
            rep   movsb
            push  es:[9*4]                         ;将原 int 9 中断例程入口地址保存到 0:200h
            pop   es:[200h]
            push  es:[9*4+2]
            pop   es:[202h]
 
    
            cli                                    ;if=0先屏蔽外部中断
            mov   word ptr es:[9*4],204h           ;将新的中断例程入口地址写入中断向量表
            mov   word ptr es:[9*4+2],0
            sti                                    ;恢复if
 
            mov   ax,4c00h
            int   21h
 
    int9:   
            push  ax
            push  bx
            push  cx
            push  es
   
            in    al,60h                           ;从端口读取键盘数据
            pushf
           
            call  dword ptr cs:[200h]              ;调用原来的 int 9 中断例程，此时 CS=0
            cmp   al,9eh                           ;比较是否收到 A 键值 1e+80h
            jne   int9ret
 
   
            mov   ax,0b800h                        ;显示一屏幕 A
            mov   es,ax
            
            mov   bx,0
            mov   cx,2000
    s:      
            mov   byte ptr es:[bx],'A'
            add   bx,2
            loop  s
    int9ret:
            pop   es
            pop   cx
            pop   bx
            pop   ax
            iret
    int9end:nop
codesg ends
end start
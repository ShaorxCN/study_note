assume cs:codesg
codesg segment
           mov  ax,2
           mov  cx,11
    s:     add  ax,ax
           loop s

           mov  ax, 4c00h
           int  21h
codesg ends
end
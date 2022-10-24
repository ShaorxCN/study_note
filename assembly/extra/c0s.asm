;取代c0s f.clink通过 也可执行
assume cs:codesg
datasg segment
           db 128 dup(0)
datasg ends
codesg segment
    start: mov  ax,datasg
           mov  ds,ax
           mov  ss,ax
           mov  sp,128
           call s
           mov  ax,4c00h
           int  21h
    s:     
codesg ends
end start
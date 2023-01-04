org 10000h
    jmp Label_Start
%include "fat12.inc"   ;类似c的include 
BaseOfKernelFile equ 0x00
OffsetOfKernelFile equ 0x100000         ;1Mb处 因为1MB一下可能并不全部可用且内核会增大

BaseTmpOfKernelAddr equ 0x00
OffsetTmpOfKernelFile equ 0x7E00       ;临时存储 因为是通过13h读取 其仅支持上限1m的寻址 所以暂时先放这儿在放到1mb以上的地方

MemoryStructBufferAddr equ 0x7E00

[SECTION .s16]
[BITS 16]                  ;告诉编译器 一下代码将会运行在16位的环境中 同理32位宽就是BITS 32
;当处于16位宽下 使用32位宽数据指令需要在指令前加入前缀0x66 使用32位宽地址指令需要加前缀0x67

Label_Start:

	mov	ax,	cs
	mov	ds,	ax
	mov	es,	ax
	mov	ax,	0x00
	mov	ss,	ax
	mov	sp,	0x7c00

;=======	display on screen : Start Loader......

	mov	ax,	1301h
	mov	bx,	000fh
	mov	dx,	0200h		;row 2
	mov	cx,	12
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	StartLoaderMessage
	int	10h

; 实模式寻址限制1mb 我们需要开启1mb以上物理地址寻址功能 并开启实模式下4GB寻址功能
;====== open address A20
    push ax
    in al,92h   ;表示从92h端口读取一个byte到al
    or al,00000010b
    out 92h,al
    pop ax

    cli               ;禁中断

    db 0x66
    lgdt [GdtPtr]  ; lgdt 从内存中读取48位的内存数据 存入DGTR寄存器

    mov eax,cr0
    or eax,1
    mov cr0,eax       ;cr0第一位置1 开启保护模式

    mov ax,SelectorData32
    mov fs,ax             ;位fs段寄存器加载新的数据段值 这样就可以通过fs获取1m以上的寻址能力
    mov eax,cr0
    and al,11111110b
    mov cr0,eax           ;退出保护模式

    sti                   ;恢复中断

;====== search kernel.bin
    mov word [SectorNo],SectorNumOfRootDirStart

Label_Search_In_Root_Dir_Begin:
    cmp word [RootDirSizeForLoop],0 
    jz Label_No_KernelBin
    dec word [RootDirSizeForLoop]
    mov ax,00h
    mov es,ax       
    mov bx,8000h    
    mov ax,[SectorNo]     
    mov cl,1               
    call Func_ReadOneSector
    mov si,LoaderFileName
    mov di,8000h     
    cld              
    mov dx,10h      
Label_Search_For_KernelBin:
    cmp dx,0         
    jz Label_Goto_Next_Sector_In_Root_Dir
    dec dx
    mov cx,11                
Label_Cmp_FileName:
    cmp cx,0
    jz Label_FileName_Found
    dec cx
    lodsb           
    cmp al,byte [es:di]  
    jz Label_Go_On       
    jmp Label_Different  
Label_Go_On:
    inc di
    jmp Label_Cmp_FileName
Label_Different:
    and di,0ffe0h       
    add di,20h          
    mov si,LoaderFileName
    jmp Label_Search_For_LoaderBin
Label_Goto_Next_Sector_In_Root_Dir:
    add word [SectorNo],1       
    jmp Label_Search_In_Root_Dir_Begin
;====== display on screen:ERROR:No KERNEL Found
Label_No_KernelBin:
    mov ax,1301h
    mov bx,008ch  
    mov dx,0100h
    mov cx,21     
    push ax
    mov ax,ds
    mov es,ax
    pop ax
    mov bp,NoLoaderMessage
    int 10h
    jmp $

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
    mov si,KernelFileName
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
    mov si,KernelFileName
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
;======  found loader.bin name in root director struct
Label_FileName_Found:
    mov ax,RootDirSectors
    and di,0ffe0h        ;到当前条目开始位置
    add di,01ah          ;根据目录结构 +26到DIR_FstClus 2个字节
    mov cx,word [es:di]  ; DIR_FstClus 保存到cx
    push cx              ;备份DIR_FstClus
    add cx,ax            ;加上目录扇区的偏移
    add cx,SectorBalance ;加上平衡值得到目标扇区
    mov eax,BaseTmpOfKernelAddr  
    mov es,eax          
    mov bx,OffsetTmpOfKernelFile  ;设置目标地址
    mov ax,cx              ;ax=文件所在开始扇区

Label_Go_On_Loading_File:
    push ax
    push bx
    mov ah,0eh           ; 准备10h的0eh功能 打印字符 此时bh=00 页码0 打印'.'
    mov al,'.'
    mov bl,0fh
    int 10h
    pop bx
    pop ax

    push	cx
	push	eax
	push	fs
	push	edi
	push	ds
	push	esi

	mov	cx,	200h
	mov	ax,	BaseOfKernelFile
	mov	fs,	ax
	mov	edi,	dword	[OffsetOfKernelFileCount]

	mov	ax,	BaseTmpOfKernelAddr
	mov	ds,	ax
	mov	esi,	OffsetTmpOfKernelFile
Label_Mov_Kernel:
    mov al,byte [ds:esi]
    mov byte [fs:edi],al
Label_File_Loaded:
    jmp BaseOfLoader:OffsetOfLoader  ;跳转到loader的部分  boot结束
;====== read one sector from floppy  ax中存放目标扇区号 cl中放需要读取的扇区个数
[SECTION .s116]
[BITS 16]
Func_ReadOneSector:
    push bp
    mov bp,sp
    ; 上面复制了sp 这边-2 相当于预留了两个byte 空间 用以下面的mov byte bp默认ss
    sub esp,2
    mov byte [bp-2],cl ;备份cl 调用方传入的读取的扇区数量
    push bx
    mov bl,[BPB_SecPerTrk]
    ;被除数在ax 商在al 余数在ah 这里ax需要调用方放入
    div bl
    inc ah ;余数+1 得到chs的扇区号 
    mov cl,ah ; 13h中断起始扇区号存储在CL
    mov dh,al ; 商放到dh
    shr al,1 ;右移一位得到柱面号(商/BPB_NumHeads)
    mov ch,al ; 获得柱面号 13h中断要求放在CH
    and dh,1 ;得到磁头号 
    pop bx  ;恢复bx
    mov dl,[BS_DrvNum];保存驱动器号
Label_Go_On_Reading:
	mov	ah,	2
	mov	al,	byte	[bp - 2]
	int	13h
	jc	Label_Go_On_Reading
	add	esp,	2
	pop	bp
	ret

;=======	get FAT Entry

Func_GetFATEntry:

	push	es
	push	bx
	push	ax
	mov	ax,	00
	mov	es,	ax
	pop	ax
	mov	byte	[Odd],	0
	mov	bx,	3
	mul	bx
	mov	bx,	2
	div	bx
	cmp	dx,	0
	jz	Label_Even
	mov	byte	[Odd],	1

Label_Even:

	xor	dx,	dx
	mov	bx,	[BPB_BytesPerSec]
	div	bx
	push	dx
	mov	bx,	8000h
	add	ax,	SectorNumOfFAT1Start
	mov	cl,	2
	call	Func_ReadOneSector
	
	pop	dx
	add	bx,	dx
	mov	ax,	[es:bx]
	cmp	byte	[Odd],	1
	jnz	Label_Even_2
	shr	ax,	4

Label_Even_2:
	and	ax,	0FFFh
	pop	bx
	pop	es
	ret
;=======	tmp variable
RootDirSizeForLoop	dw	RootDirSectors
SectorNo		dw	0
Odd			db	0
OffsetOfKernelFileCount	dd	OffsetOfKernelFile

MemStructNumber		dd	0

SVGAModeCounter		dd	0

DisplayPosition		dd	0

;=======	display messages

StartLoaderMessage:	db	"Start Loader"
NoLoaderMessage:	db	"ERROR:No KERNEL Found"
KernelFileName:		db	"KERNEL  BIN",0
StartGetMemStructMessage:	db	"Start Get Memory Struct (address,size,type)."
GetMemStructErrMessage:	db	"Get Memory Struct ERROR"
GetMemStructOKMessage:	db	"Get Memory Struct SUCCESSFUL!"

StartGetSVGAVBEInfoMessage:	db	"Start Get SVGA VBE Info"
GetSVGAVBEInfoErrMessage:	db	"Get SVGA VBE Info ERROR"
GetSVGAVBEInfoOKMessage:	db	"Get SVGA VBE Info SUCCESSFUL!"

StartGetSVGAModeInfoMessage:	db	"Start Get SVGA Mode Info"
GetSVGAModeInfoErrMessage:	db	"Get SVGA Mode Info ERROR"
GetSVGAModeInfoOKMessage:	db	"Get SVGA Mode Info SUCCESSFUL!"

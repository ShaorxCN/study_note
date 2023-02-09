org 10000h
    jmp Label_Start
%include "fat12.inc"   ;类似c的include 
BaseOfKernelFile equ 0x00
OffsetOfKernelFile equ 0x100000         ;1Mb处 因为1MB一下可能并不全部可用且内核会增大

BaseTmpOfKernelAddr equ 0x00
OffsetTmpOfKernelFile equ 0x7E00       ;临时存储 因为是通过13h读取 其仅支持上限1m的寻址 所以暂时先放这儿再放到1mb以上的地方

MemoryStructBufferAddr equ 0x7E00
; :表示地址  不带:直接值
; 定义GDT表 空(必须)+2个 共三个表项
[SECTION gdt]
LABEL_GDT:		dd	0,0        ;空描述符
LABEL_DESC_CODE32:	dd	0x0000FFFF,0x00CF9A00
LABEL_DESC_DATA32:	dd	0x0000FFFF,0x00CF9200

GdtLen	equ	$ - LABEL_GDT          ;当前行编译后的地址到LABEL_GDT的偏移GDT表的长度
GdtPtr	dw	GdtLen - 1             ;GDT界限   低位 表示长度
	    dd	LABEL_GDT              ;GDT基地址    这个变量6字节 2+4  16+32 用来放入GDTR

SelectorCode32	equ	LABEL_DESC_CODE32 - LABEL_GDT   ;8
SelectorData32	equ	LABEL_DESC_DATA32 - LABEL_GDT   ;16?

[SECTION gdt64]

LABEL_GDT64:		dq	0x0000000000000000
LABEL_DESC_CODE64:	dq	0x0020980000000000
LABEL_DESC_DATA64:	dq	0x0000920000000000

GdtLen64	equ	$ - LABEL_GDT64
GdtPtr64	dw	GdtLen64 - 1
		    dd	LABEL_GDT64

SelectorCode64	equ	LABEL_DESC_CODE64 - LABEL_GDT64
SelectorData64	equ	LABEL_DESC_DATA64 - LABEL_GDT64

[SECTION .s16]
[BITS 16]                  ;告诉编译器 以下代码将会运行在16位的环境中 同理32位宽就是BITS 32
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
    mov fs,ax             ;为fs段寄存器加载新的数据段值 这样就可以通过fs获取1m以上的寻址能力
    mov eax,cr0
    and al,11111110b
    mov cr0,eax           ;退出保护模式

    sti                   ;恢复中断
;=======	reset floppy
	xor	ah,	ah
	xor	dl,	dl
	int	13h
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
;======  found kernel.bin name in root director struct
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
    int 10h             ;输出调用
    pop bx
    pop ax

    mov cl,1
    call Func_ReadOneSector     ;先读到 OffsetTmpOfKernelFile
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
	mov	esi,	OffsetTmpOfKernelFile       ;获取临时存放得位置
Label_Mov_Kernel:
    mov al,byte [ds:esi]                    ;一个字节一个字节得转移到1m得位置
    mov byte [fs:edi],al

    inc esi
    inc edi

    loop Label_Mov_Kernel

    mov eax, 0x1000
    mov ds,eax

    mov dword [OffsetOfKernelFileCount],edi

    pop esi
    pop ds
    pop edi
    pop fs
    pop eax
    pop ex

    call Func_GetFATEntry
    cmp ax,0FFFh
    jz Label_File_Loaded  ;是的话代表加载完成  类似je
    push ax
    mov dx,RootDirSectors
    add ax,dx
    add ax,SectorBalance
    jmp Label_Go_On_Loading_File
Label_File_Loaded:
    mov ax,0B800h
    mov gs,ax
    mov ah,0fh
    mov al,'G'
    mov [gs:((80*0+39)*2)],ax ;屏幕第0行 第39列
KillMotor:                       ;关闭软驱
    push dx
    mov dx,03F2h
    mov al,0
    out dx,al
    pop dx
;======= get memory address size type
    mov	ax,	1301h
	mov	bx,	000Fh
	mov	dx,	0400h		;row 4
	mov	cx,	24
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	StartGetMemStructMessage       
	int	10h                              ;打印信息 开始获取内存结构

	mov	ebx,	0
	mov	ax,	0x00
	mov	es,	ax
	mov	di,	MemoryStructBufferAddr	
Label_Get_Mem_Struct:
    mov eax,0x0E820
    mov ecx,20
    mov edx,0x534D4150  ;SMAP
; 向es:di填充ecx字节的数据  不过通常就是20 不论ecx多少
; cf=0表示调用成功 否则错误  ebx存放下个内存的描述符 已被第二次调用 第一次ebx必须是0 
    int 15h             
    jc Label_Get_Mem_Fail
    add di,20                    ; 先准备好下次需要存储的位置 
    cmp ebx,0                    ;0表示结束了
    jne Label_Get_Mem_Struct     ;没有结束就继续调用
    jmp Label_Get_Mem_OK               
Label_Get_Mem_Fail:
    mov	ax,	1301h
	mov	bx,	008Ch
	mov	dx,	0500h		;row 5
	mov	cx,	23
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	GetMemStructErrMessage
	int	10h
	jmp	$
Label_Get_Mem_OK:
	mov	ax,	1301h
	mov	bx,	000Fh
	mov	dx,	0600h		;row 6,0 col
	mov	cx,	29
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	GetMemStructOKMessage ;es:bp
	int	10h
;======= get SVGA information 10h ax=4F00  SVGA=super VGA
    mov ax,1301h
    mov bx,000Fh
    mov dx,0800h       ;row 8
    mov cx,23
    push ax
    mov ax,ds
    mov es,ax
    pop ax
    mov bp,StartGetSVGAVBEInfoMessage
    int 10h

    mov ax,0x00
    mov es,ax
    mov di,0x8000
    mov ax,4F00h

    int 10h

    cmp ax,004Fh
    jz .KO
;======== Fail
    mov ax,1301h
    mov bx,008Ch
    mov dx,0900h
    mov cx,23
    push ax
    mov ax,ds
    mov es,ax
    pop ax
    mov bp,GetSVGAVBEInfoErrMessage
    int 10h

    jmp $
.KO:
    mov ax,1301h ; al bit1=0 则属性在bl
    mov bx,000Fh ;page 0     白色加亮
    mov dx,0A00h  ;row 10
    mov cx,29
    push ax
    mov ax,ds
    mov es,ax
    pop ax
    mov bp,GetSVGAModeInfoOKMessage
    int 10h
;======= Get SVGA Mode info
    mov ax,1301h
    mov bx,000Fh
    mov dx,0C00h
    mov cx,24
    push ax
    mov ax,ds
    mov es,ax
    pop ax
    mov bp,StartGetSVGAModeInfoMessage
    int 10h

    mov ax,0x00
    mov es,ax
    mov si,0x800e
    mov esi,dword [es:si]
    mov edi,0x8200
Label_SVGA_Mode_Info_Get:
    mov cx,word [es:esi]
;======= display SVGA mode information
    push ax
    mov ax,00h
    mov al,ch
    call Label_DispAL

    mov ax,00h
    mov al,cl
    call Label_DispAL

    pop ax
;====== 
    cmp	cx,	0FFFFh
	jz	Label_SVGA_Mode_Info_Finish

	mov	ax,	4F01h
	int	10h

	cmp	ax,	004Fh

	jnz	Label_SVGA_Mode_Info_FAIL	

	add	esi,	2
	add	edi,	0x100

	jmp	Label_SVGA_Mode_Info_Get
Label_SVGA_Mode_Info_FAIL:
	mov	ax,	1301h
	mov	bx,	008Ch
	mov	dx,	0D00h		;row 13
	mov	cx,	24
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	GetSVGAModeInfoErrMessage
	int	10h

Label_SET_SVGA_Mode_VESA_VBE_FAIL:
	jmp	$
Label_SVGA_Mode_Info_Finish:

	mov	ax,	1301h
	mov	bx,	000Fh
	mov	dx,	0E00h		;row 14
	mov	cx,	30
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	GetSVGAModeInfoOKMessage
	int	10h
;====== set SVGA information  设置视频图像芯片的显示模式
    mov ax,4F02h
    mov bx,4180h  ;==========================mode: 0x180 or 0x143
    int 10h

    cmp ax,004Fh  ;ah=0 成功 否则失败 al=4fH 代表功能支持
    jnz Label_SET_SVGA_Mode_VESA_VBE_FAIL
;=======	init IDT GDT goto protect mode 

	cli			; 关可屏蔽中断响应

	db	0x66                    ;LGDT 和 LIDT指令前缀 修饰当前指令操作数是32位宽
	lgdt	[GdtPtr]

;	db	0x66
;	lidt	[IDT_POINTER]

	mov	eax,	cr0         ;位0使PE 位31是PG
	or	eax,	1
	mov	cr0,	eax	        ;置位pe

	jmp	dword SelectorCode32:GO_TO_TMP_Protect

[SECTION .s32]
[BITS 32]

GO_TO_TMP_Protect:
;=======	go to tmp long mode
	mov	ax,	0x10
	mov	ds,	ax
	mov	es,	ax
	mov	fs,	ax
	mov	ss,	ax
	mov	esp,	7E00h

	call	support_long_mode
	test	eax,	eax    ;测试eax eax按位and

	jz	no_support         ;zf=1 也就说运算结果位0  跳转no support  
;=======	init temporary page table 0x90000  支持就初始化临时页表  页表项8字节   TODO
	mov	dword	[0x90000],	0x91007
	mov	dword	[0x90800],	0x91007		

	mov	dword	[0x91000],	0x92007

	mov	dword	[0x92000],	0x000083

	mov	dword	[0x92008],	0x200083

	mov	dword	[0x92010],	0x400083

	mov	dword	[0x92018],	0x600083

	mov	dword	[0x92020],	0x800083

	mov	dword	[0x92028],	0xa00083
;=======	load GDTR  
; 段寄存器的重置 cs依靠跳转执行
	db	0x66
	lgdt	[GdtPtr64]
	mov	ax,	0x10
	mov	ds,	ax
	mov	es,	ax
	mov	fs,	ax
	mov	gs,	ax
	mov	ss,	ax

	mov	esp,	7E00h
;=======	open PAE

	mov	eax,	cr4
	bts	eax,	5                   ;将eax的5bit取出放入cf 然后将5bit位置1 也就是PAE位
	mov	cr4,	eax
;=======	load	cr3
; 手动设置cr3
	mov	eax,	0x90000
	mov	cr3,	eax
;=======	enable long-mode
; 置位LME 激活IA-32e模式
	mov	ecx,	0C0000080h		;IA32_EFER 通过向ECX传递寄存器地址来实现MSR寄存器组的读写。MSR寄存器是EDX:EAX组成的64位寄存器 EDX高32；必须0特权级下执行
	rdmsr

	bts	eax,	8
	wrmsr
;=======	open PE and paging
	mov	eax,	cr0
	bts	eax,	0
	bts	eax,	31
	mov	cr0,	eax

	jmp	SelectorCode64:OffsetOfKernelFile
;=======	test support long mode or not
support_long_mode:
	mov	eax,	0x80000000     ;执行完cpuid后 eax中存放的是拓展信息功能代码的最大值
; CPUID操作码是一个面向x86架构的处理器补充指令，它的名称派生自CPU识别，
; 作用是允许软件发现处理器的详细信息。它由英特尔在1993年引入奔腾和SL增强486处理器。
; 默认使用eax来存储执行前的功能号  参考 https://www.felixcloutier.com/x86/cpuid
	cpuid                     
	cmp	eax,	0x80000001  ;只有当cpu支持的功能代码大于这个时才有可能支持长模式
	setnb	al	                      ;是否大于等于  就置位al 1 否则al 0
	jb	support_long_mode_done        ;小于就跳转
	mov	eax,	0x80000001            ; eax ebx 保留 ecx edx则事故拓展功能位信息
	cpuid
	bt	edx,	29                    ;edx从29bit的位置 拷贝到carry flag 因为拓展功能项0x80000001的第29表示是否支持IA-32e
	setc	al                        ;set byte if cf=1  也就是说支持的话将al设置为1
support_long_mode_done:              
	movzx	eax,	al                 ;使用0填充eax剩下的位  这里相当于只有al位置保留其他置零 小于的时候al时0 大于al是1
	ret
;=======	no support
no_support:
	jmp	$
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
;====== display num in al 直接写0x0b800的方法
Label_DispAL:
    push ecx
    push edx
    push edi          ;备份寄存器

    mov edi,[DisplayPosition]  ;0b800后的准备偏移值
    mov ah,0FH   ;黑底白字
    mov dl,al    ;保存al
    shr al,4     ;这里先展示al的高四位
    mov ecx,2    ;loop次数
.begin:
    and al,0Fh  ;同样是为先展示al的高4为准备 置零
    cmp al,9    ;这边按照16进制展示 所以做个转换  大于9就是字母a-f了
    ja .1
    add al,'0'
    jmp .2
.1:
    sub  al,0Ah
    add al,'A'
.2:
    mov [gs:edi],ax
    add edi,2
    mov al,dl     ; 之前的第四位回来 通过loop展示
    loop .begin   ;先减1判断再是否执行 
    mov [DisplayPosition],edi

    pop edi
    pop edx
    pop ecx
    ret
;=======	tmp IDT
;在切换前已经cli关中断了 所以只需要初始化结构就行 如果可以保证切换过程中无中断 没有IDT也可以
; DQ 四字 八字节  DT define ten bytes
;IDT表项 保护模式下8字节
IDT:
	times	0x50	dq	0   ;80X8 640字节
IDT_END:

IDT_POINTER:
		dw	IDT_END - IDT - 1
		dd	IDT
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
StartGetMemStructMessage:	db	"Start Get Memory Struct."
GetMemStructErrMessage:	db	"Get Memory Struct ERROR"
GetMemStructOKMessage:	db	"Get Memory Struct SUCCESSFUL!"

StartGetSVGAVBEInfoMessage:	db	"Start Get SVGA VBE Info"
GetSVGAVBEInfoErrMessage:	db	"Get SVGA VBE Info ERROR"
GetSVGAVBEInfoOKMessage:	db	"Get SVGA VBE Info SUCCESSFUL!"

StartGetSVGAModeInfoMessage:	db	"Start Get SVGA Mode Info"
GetSVGAModeInfoErrMessage:	db	"Get SVGA Mode Info ERROR"
GetSVGAModeInfoOKMessage:	db	"Get SVGA Mode Info SUCCESSFUL!"

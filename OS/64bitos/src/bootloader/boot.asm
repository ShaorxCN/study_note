;规定程序存放起始地址 主要是为了计算程序中的段内偏移量。 cs启动一般默认为0  BIOS直接类似 jmp:0:7c00h
;比如偏移量是4B 不加org默认是0000但是boot默认加载到7c00那么当还是用0000+4b肯定访问不到7C4B.一般相对是程序放置位置+相对偏移量定址
;编译阶段只能得出相对偏移地址，自然是程序开始处为零点算偏移，只有实际运行加载才能得出程序放置位置，但是某些固定 比如引导现阶段肯定是7c00 可以通过org直接指定
;当然这里也可以通过DS/ES设置为7c0h实现可以不带org
org 0x7c00   
BaseOfStack equ 0x7c00 ;BaseOfStack 标识符 等价于 0x7c00  equ指令 让左边等于右边 但是不会为标识符分配空间(汇编编译器编译时自动替换 所以不占空间这边自然也不会说干扰mbr) 标识符无法重名 无法重新定义。
BaseOfLoader equ 0x1000
OffsetOfLoader equ 0x00

RootDirSectors equ 14        ; 目录区占14个扇区
SectorNumOfRootDirStart equ 19;1+9+9 = 19   ;这里参考LBA和CHS的区别 LBA线性描述 扇区从0开始CHS1开始 因为同密度导致扇区数不一致[柱面就是磁道 内向外 越来越大]
SectorNumOfFAT1Start equ 1  ; FAT1开始扇区号
;平衡目录和数据区初始簇号的插值 这里一个扇区就是一个簇 然后标准里数据区起始簇号是2 所以减了2  
;数据区地址=根目录起始扇区号+根目录所占扇区数+值-2 -2放到前面就是17
SectorBalance equ 17  

    jmp short Label_Start
    nop
    BS_OEMName db 'MINEboot'        ;一定要放在这儿 代表软盘的描述信息 参见介绍里的引导扇区结构图
    BPB_BytesPerSec dw 512
    BPB_SecPerClus db 1
    BPB_RevdSecCnt dw 1
    BPB_NumFATs db 2
    BPB_RootEntCnt dw 224
    BPB_TotSec16 dw 2880
    BPB_Media db  0xf0
    BPB_FATSz16 dw 9
    ; 每个磁道的扇区数
    BPB_SecPerTrk dw 18
    BPB_NumHeads dw 2
    BPB_hiddSec dd 0
    BPB_TotSec32 dd 0
    BS_DrvNum db 0
    BS_Reserved1 db 0
    BS_BootSig db 29h
    BS_VolID dd 0
    BS_VolLab db 'boot loader'   ; 卷标 11字节
    BS_FileSysType db 'FAT12   ' ;文件系统类型  8字节

Label_Start:
    mov ax,cs   ;初始化ds es  ss
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov sp,BaseOfStack

;====== clear screen

    mov ax,0600h
    mov bx,0700h
    mov cx,0
    mov dx,0184fh
    ;屏幕相关操作  主功能编号放在AH中  这里是先是06h 也就是上卷指定范围的窗口（07下卷）  滚动行数 AL 这里是0 则是清屏 下面寄存器的值其实无效
    ;BH则是滚动后空出位置放入的属性 
    ;CH CL分别是窗口的左上角坐标(y,x)
    ;(DH,DL)右下角位置 y,x这里是坐标(4f,18) 
    ;屏幕坐标系原点是屏幕左上角
    int 10h         
    
;====== set focus
    mov ax,0200h
    mov bx,0000h
    mov dx,0000h
    int 10h         ; AH= 02h  也就是设置光标位置  BH是页码   DH:行 或者 y坐标  这里是0  DL是列 也是0
;====== display on screen;Start Booting......
    mov ax,1301h
    mov bx,000fh
    mov dx,0000h
    mov cx,10
    push ax
    mov ax,ds
    mov es,ax
    pop ax
    mov bp,StartBootMessage
    ; 13h  字符串的显示功能
    ; AL 写入模式：00h 字符串属性BL提供 CX是字符串长度byte 光标位置不变
    ;             01h 和00h一致 但是光标移动到字符串尾端位置
    ;             02h 字符串属性由每个字符后面紧跟的字节提供 CX存储的字符串长度变为word为单位 也就是2x 显示后光标位置不变
    ;             03h 同02h但是光标移动到末尾
    ; CX=字符串长度 DH 游标的坐标行号 DL 游标的坐标列号  ES:BP 要显示的字符串内存地址  BH页码 BL字符属性
    int 10h

;======  rset floppy
    
    xor ah,ah
    xor dl,dl;(DH,DL)右下角位置 y,x这里是坐标(4f,18) 
    ; int 13h  AH=00h 重置磁盘驱动器  DL=驱动器号 00H~7FH：软盘  80H~0FFH:硬盘
    ;       DL=00h 代表第一个软盘驱动器 drive A:
    ;          01h       第二个 drive B:
    ;          80h       第一个硬盘驱动器
    int 13h
;====== search loader.bin
    mov word [SectorNo],SectorNumOfRootDirStart

Label_Search_In_Root_Dir_Begin:
    ;这里的值和目录占扇区数一致 检查是否查找完成
    cmp word [RootDirSizeForLoop],0 
    ;zf=1 说明为0  遍历结束 跳转到未找到loaderbin逻辑
    jz Label_No_LoaderBin
    dec word [RootDirSizeForLoop]
    mov ax,00h
    mov es,ax       ;es:bx是读取扇区的缓冲区地址
    mov bx,8000h    ;也就是08000h
    mov ax,[SectorNo]     ;待读取扇区编号
    mov cl,1               ;读取一个扇区
    call Func_ReadOneSector
    mov si,LoaderFileName
    mov di,8000h     ;读取的缓冲区设置给di
    cld              ;df=0  si向高增长
    mov dx,10h       ;比较次数 一个扇区共512/32=16 个目录项
Label_Search_For_LoaderBin:
    cmp dx,0         ;这个扇区比较完了 就跳转到读取下一个目录扇区
    jz Label_Goto_Next_Sector_In_Root_Dir
    dec dx
    mov cx,11                ;循环控制  11字节 每个目录只需要对比11个字节
Label_Cmp_FileName:
    cmp cx,0
    jz Label_FileName_Found
    dec cx
    lodsb          ;根据si读入AL df=0  si增加 
    cmp al,byte [es:di]  ;ds:si 和 es:di 的比较
    jz Label_Go_On       ;一样就比较下一个字节
    jmp Label_Different  ;不一样跳出
Label_Go_On:
    inc di
    jmp Label_Cmp_FileName
Label_Different:
    and di,0ffe0h       ;低于20h的位归零 相当于 指向本条目的开头
    add di,20h          ;+20h 跳转到下一个条目
    mov si,LoaderFileName
    jmp Label_Search_For_LoaderBin
Label_Goto_Next_Sector_In_Root_Dir:
    add word [SectorNo],1       ;待读取扇区编号+1
    jmp Label_Search_In_Root_Dir_Begin
;====== display on screen:ERROR:No LOADER Found
Label_No_LoaderBin:
    mov ax,1301h
    mov bx,008ch  ;页码0 属性 8c 10001100 红色
    mov dx,0100h
    mov cx,21     ;字符串长度21
    push ax
    mov ax,ds
    mov es,ax
    pop ax
    mov bp,NoLoaderMessage
    ; 依旧是10h打印信息 并且后面死循环 ah=13 al=01
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
    mov ax,BaseOfLoader  
    mov es,ax          
    mov bx,OffsetOfLoader  ;设置目标地址
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

    mov cl,1
    call Func_ReadOneSector    ;读取一个扇区
    pop ax                     ;此时ax = DIR_FstClus的值
    call Func_GetFATEntry      ;查看表项 是否需要继续读取
    cmp ax,0fffh               ;是否是最后一个簇 是的话就结束
    jz Label_File_Loaded
    push ax
    mov dx,RootDirSectors
    add ax,dx
    add ax,SectorBalance
    add bx,[BPB_BytesPerSec]
    jmp Label_Go_On_Loading_File
Label_File_Loaded:
    jmp BaseOfLoader:OffsetOfLoader  ;跳转到loader的部分  boot结束
;====== read one sector from floppy  ax中存放目标扇区号 cl中放需要读取的扇区个数
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
    mov ah,2
    mov al,byte [bp-2];之前的cl保存到al 读取al个扇区 
    int 13h   ; ah=2 表示读扇区  ch柱面 cl扇区 dh磁头 dl=驱动器
    ;jc = jump if Carry 读取错误则CF=1 那就继续跳转到Label_Go_On_Reading
    jc Label_Go_On_Reading
    add esp,2
    pop bp
    ret
;======  get FAT Entry 每个表项1.5B
Func_GetFATEntry:
    push es
    push bx
    push ax
    mov ax,00
    mov es,ax
    pop ax             ;ax 是DIR_FstClus 也可以说是表项号
    mov byte [Odd],0  ;先标记为偶数
    mov bx,3
    mul bx            ;ax * 3
    mov bx,2
    div bx            ;ax\2 这边*3\2 因为一个占1.5B所以乘以表项号得出所需空间 余数代表还占了后面半个字节 则该表项从中间开始
    cmp dx,0          ;比较余数 如果为0则是偶数 否则奇数
    jz Label_Even
    mov byte [Odd],1
Label_Even:
    xor dx,dx        ;dx清零
    mov bx,[BPB_BytesPerSec]
    div bx            ;偏移量/扇区字节数 得出扇区偏移量 余数代表在扇区内的偏移字节
    push dx           ;保存余数 也就是在表项在扇区内的偏移量
    mov bx,8000h       ;之前的数据读取的缓冲位置 es:bx
    add ax,SectorNumOfFAT1Start     ;目录开始扇区是1加上得出最终扇区号
    mov cl,2                        ;一次读取两个扇区  防止表项跨扇区
    call Func_ReadOneSector

    pop dx
    add bx,dx
    mov ax,[es:bx]  ;数据读入ax 2字节
    cmp byte [Odd],1  ;如果是奇数代表从开头开始 需要特殊处理
    jnz Label_Even2   ;偶数则直接跳转
    shr ax,4        ;奇数右移4位 小端序 然后第四位被上个表项占了
Label_Even2:
    and ax,0fffh    ;偶数读取  屏蔽前四位 是下个表项或者移位出来的
    pop bx
    pop es
    ret
;====== tmp variable
RootDirSizeForLoop dw RootDirSectors
SectorNo dw 0
Odd  db 0        ;FAT表项12bit 就是1.5B 所以需要纪录奇偶

;====== display messages
StartBootMessage: db "Start Boot"
NoLoaderMessage: db "ERROR:No LOADER Found"
LoaderFileName: db "LOADER  BIN",0

;====== file zero until whole sector
    ; $-$$ 表示当前行被编译后的地址减去本节程序的起始地址 也就是相当于计算当前程序生成的机器码的长度 
    ; 510-结果则是说明引导程序需要填充的数据长度。因为读取都是按照扇区读取的 也就是512全部都读。times 则是重复多次操作 就是填充
    times 510-($ - $$) db 0
    ;Intel处理器是小端序 所以存储 0x55 0xaa就是 0xaa55 小端序就是低字节的放在前面 也就是低内存位置  这边检测到0磁头0磁道1扇区以0x55aa结尾就认为是引导程序 加载到0x7c00处开始执行
    dw 0xaa55


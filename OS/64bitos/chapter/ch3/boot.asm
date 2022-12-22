;规定程序存放起始地址 主要是为了计算程序中的段内偏移量。 cs启动一般默认为0  BIOS直接类似 jmp:0:7c00h
;比如偏移量是4B 不加org默认是0000但是boot默认加载到7c00那么当还是用0000+4b肯定访问不到7C4B.一般相对是程序放置位置+相对偏移量定址
;编译阶段只能得出相对偏移地址，自然是程序开始处为零点算偏移，只有实际运行加载才能得出程序放置位置，但是某些固定 比如引导现阶段肯定是7c00 可以通过org直接指定
;当然这里也可以通过DS/ES设置为7c0h实现可以不带org
org 0x7c00   
BaseOfStack equ 0x7c00 ;BaseOfStack 标识符 等价于 0x7c00  equ指令 让左边等于右边 但是不会为标识符分配空间 标识符无法重名 无法重新定义。

label_Start:
    mov ax,cs
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

jmp $

StartBootMessage: db "Start Boot"
;====== file zero until whole sector
; $-$$ 表示当前行被编译后的地址减去本节程序的起始地址 也就是相当于计算当前程序生成的机器码的长度 
; 510-结果则是说明引导程序需要填充的数据长度。因为读取都是按照扇区读取的 也就是512全部都读。times 则是重复多次操作 就是填充
times 510-($ - $$) db 0
;Intel处理器是小端序 所以存储 0x55 0xaa就是 0xaa55 小端序就是低字节的放在前面 也就是低内存位置 
dw 0xaa55

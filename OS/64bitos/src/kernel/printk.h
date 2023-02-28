#ifndef __PRINTK_H__
#define __PRINTK_H__

#include <stdarg.h>
#include "font.h"
#include "linkage.h"

#define ZEROPAD	1		/* pad with zero */
#define SIGN	2		/* unsigned/signed long */
#define PLUS	4		/* show plus */
#define SPACE	8		/* space if plus */
#define LEFT	16		/* left justified */
#define SPECIAL	32		/* 0x */
#define SMALL	64		/* use 'abcdef' instead of 'ABCDEF' */

#define is_digit(c)	((c) >= '0' && (c) <= '9')

#define WHITE 	0x00ffffff		//白
#define BLACK 	0x00000000		//黑
#define RED	    0x00ff0000		//红
#define ORANGE	0x00ff8000		//橙
#define YELLOW	0x00ffff00		//黄
#define GREEN	0x0000ff00		//绿
#define BLUE	0x000000ff		//蓝
#define INDIGO	0x0000ffff		//靛
#define PURPLE	0x008000ff		//紫

/*

*/

extern unsigned char font_ascii[256][16];

char buf[4096]={0};

// 全局变量
// 记录分辨率 光标位置 字符像素矩阵尺寸 帧缓存区起始地址和容量大小的结构变量Pos
struct position
{
	// 分辨率 1440*900
	int XResolution; 
	int YResolution;

	// 光标位置
	int XPosition;
	int YPosition;
	
	// 矩阵大小
	int XCharSize;
	int YCharSize;

	// 起始地址和容量
	unsigned int * FB_addr;
	unsigned long FB_length;
}Pos;


/*

*/

void putchar(unsigned int * fb,int Xsize,int x,int y,unsigned int FRcolor,unsigned int BKcolor,unsigned char font);

/*

*/

int skip_atoi(const char **s);

/*
   内嵌汇编
   指令部分:输出部分:输入部分:损坏部分 
   这里指令是divq %rcx
   输出部分是n和__res 也就是商和余数 "=a" "=d"表示指定了输出的寄存器值 且是只读取 因为指令本身余数是放在rdx所以__res和rdx关联
   输入部分是n,0,base divq是RDX:RAX做被除数 因为long最多8B所以RDX置0  c指定rcx 读入base
   
*/

#define do_div(n,base) ({ \
int __res; \
__asm__("divq %%rcx":"=a" (n),"=d" (__res):"0" (n),"1" (0),"c" (base)); \
__res; })

/*

*/

static char * number(char * str, long num, int base, int size, int precision ,int type);

/*

*/

int vsprintf(char * buf,const char *fmt, va_list args);

/*

*/

int color_printk(unsigned int FRcolor,unsigned int BKcolor,const char * fmt,...);

#endif


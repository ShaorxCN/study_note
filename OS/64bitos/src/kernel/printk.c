#include <stdarg.h>
#include "printk.h"
#include "lib.h"
#include "memory.h"

// 重新映射vbe缓存帧
void frame_buffer_init()
{
	////re init frame buffer;
	unsigned long i;
	unsigned long *tmp;
	unsigned long *tmp1;
	unsigned int *FB_addr = (unsigned int *)Phy_To_Virt(0xe0000000);

	Global_CR3 = Get_gdt();

	tmp = Phy_To_Virt((unsigned long *)((unsigned long)Global_CR3 & (~0xfffUL)) + (((unsigned long)FB_addr >> PAGE_GDT_SHIFT) & 0x1ff));
	if (*tmp == 0)
	{
		unsigned long *virtual = kmalloc(PAGE_4K_SIZE, 0);
		set_mpl4t(tmp, mk_mpl4t(Virt_To_Phy(virtual), PAGE_KERNEL_GDT));
	}

	tmp = Phy_To_Virt((unsigned long *)(*tmp & (~0xfffUL)) + (((unsigned long)FB_addr >> PAGE_1G_SHIFT) & 0x1ff));
	if (*tmp == 0)
	{
		unsigned long *virtual = kmalloc(PAGE_4K_SIZE, 0);
		set_pdpt(tmp, mk_pdpt(Virt_To_Phy(virtual), PAGE_KERNEL_Dir));
	}

	for (i = 0; i < Pos.FB_length; i += PAGE_2M_SIZE)
	{
		tmp1 = Phy_To_Virt((unsigned long *)(*tmp & (~0xfffUL)) + (((unsigned long)((unsigned long)FB_addr + i) >> PAGE_2M_SHIFT) & 0x1ff));

		unsigned long phy = 0xe0000000 + i;
		set_pdt(tmp1, mk_pdt(phy, PAGE_KERNEL_Page | PAGE_PWT | PAGE_PCD));
	}

	Pos.FB_addr = (unsigned int *)Phy_To_Virt(0xe0000000);

	flush_tlb();
}

/*
	输出指定字符
	帧起始地址，分辨率x,打印矩阵的x和y开始地址  字符颜色，背景色，字符
*/
void putchar(unsigned int *fb, int Xsize, int x, int y, unsigned int FRcolor, unsigned int BKcolor, unsigned char font)
{
	int i = 0, j = 0;
	unsigned int *addr = NULL;
	unsigned char *fontp = NULL;
	int testval = 0;
	fontp = font_ascii[font];
	// 16行
	for (i = 0; i < 16; i++)
	{
		addr = fb + Xsize * (y + i) + x;
		testval = 0x100; // 0001 0000 0000 下面先右移 也就是 0000 1000 0000 就是从bit[8]开始看有没有1 一位一位的看
		for (j = 0; j < 8; j++)
		{
			testval = testval >> 1;
			if (*fontp & testval)
				*addr = FRcolor;
			else
				*addr = BKcolor;
			addr++;
		}
		fontp++;
	}
}

/*
	转换为int
*/

int skip_atoi(const char **s)
{
	int i = 0;
	// 如果是数字则该值=前值*10加后值
	// s++ 先取值再++
	while (is_digit(**s))
		i = i * 10 + *((*s)++) - '0';
	return i;
}

/*
	字母转换为指定进制规格的字符串 base进制(2-36) size长度  precision精度 type格式控制
*/

static char *number(char *str, long num, int base, int size, int precision, int type)
{
	char c, sign, tmp[50];
	const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int i;

	if (type & SMALL)
		digits = "0123456789abcdefghijklmnopqrstuvwxyz";
	if (type & LEFT)
		type &= ~ZEROPAD; // 左对齐无0填充
	if (base < 2 || base > 36)
		return 0;
	c = (type & ZEROPAD) ? '0' : ' ';
	sign = 0;
	if (type & SIGN && num < 0)
	{
		sign = '-';
		num = -num;
	}
	else
		sign = (type & PLUS) ? '+' : ((type & SPACE) ? ' ' : 0);
	if (sign)
		size--;
	if (type & SPECIAL)
		if (base == 16)
			size -= 2;
		else if (base == 8)
			size--;
	i = 0;
	if (num == 0)
		tmp[i++] = '0';
	else
		while (num != 0)
			tmp[i++] = digits[do_div(num, base)];
	if (i > precision)
		precision = i;
	size -= precision;
	if (!(type & (ZEROPAD + LEFT)))
		while (size-- > 0)
			*str++ = ' ';
	if (sign)
		*str++ = sign;
	if (type & SPECIAL)
		if (base == 8)
			*str++ = '0';
		else if (base == 16)
		{
			*str++ = '0';
			*str++ = digits[33];
		}
	if (!(type & LEFT))
		while (size-- > 0)
			*str++ = c;

	while (i < precision--)
		*str++ = '0';
	while (i-- > 0)
		*str++ = tmp[i];
	while (size-- > 0)
		*str++ = ' ';
	return str;
}

/*
	解析格式 返回实际元素个数
*/

int vsprintf(char *buf, const char *fmt, va_list args)
{
	char *str, *s;
	int flags;
	int field_width;
	int precision;
	int len, i;

	int qualifier; /* 'h', 'l', 'L' or 'Z' for integer fields */

	// 解析格式化字符串
	for (str = buf; *fmt; fmt++)
	{
		// 检查占位符
		if (*fmt != '%')
		{
			*str++ = *fmt;
			continue;
		}
		flags = 0;
	repeat:
		fmt++;
		// 标志格式占位符解析
		switch (*fmt)
		{
		case '-':
			flags |= LEFT;
			goto repeat;
		case '+':
			flags |= PLUS;
			goto repeat;
		case ' ':
			flags |= SPACE;
			goto repeat;
		case '#':
			flags |= SPECIAL;
			goto repeat;
		case '0':
			flags |= ZEROPAD;
			goto repeat;
		}

		/* get field width */

		field_width = -1;
		if (is_digit(*fmt))
			field_width = skip_atoi(&fmt);
		else if (*fmt == '*')
		{
			fmt++;
			field_width = va_arg(args, int);
			if (field_width < 0)
			{
				field_width = -field_width;
				flags |= LEFT;
			}
		}

		/* get the precision */

		precision = -1;
		if (*fmt == '.')
		{
			fmt++;
			if (is_digit(*fmt))
				precision = skip_atoi(&fmt);
			else if (*fmt == '*')
			{
				fmt++;
				precision = va_arg(args, int);
			}
			if (precision < 0)
				precision = 0;
		}

		qualifier = -1;
		if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L' || *fmt == 'Z')
		{
			qualifier = *fmt;
			fmt++;
		}

		switch (*fmt)
		{
		case 'c':

			if (!(flags & LEFT))
				while (--field_width > 0)
					*str++ = ' ';
			*str++ = (unsigned char)va_arg(args, int);
			while (--field_width > 0)
				*str++ = ' ';
			break;

		case 's':

			s = va_arg(args, char *);
			if (!s)
				s = '\0';
			len = strlen(s);
			if (precision < 0)
				precision = len;
			else if (len > precision)
				len = precision;

			if (!(flags & LEFT))
				while (len < field_width--)
					*str++ = ' ';
			for (i = 0; i < len; i++)
				*str++ = *s++;
			while (len < field_width--)
				*str++ = ' ';
			break;

		case 'o':

			if (qualifier == 'l')
				str = number(str, va_arg(args, unsigned long), 8, field_width, precision, flags);
			else
				str = number(str, va_arg(args, unsigned int), 8, field_width, precision, flags);
			break;

		case 'p':

			if (field_width == -1)
			{
				field_width = 2 * sizeof(void *);
				flags |= ZEROPAD;
			}

			str = number(str, (unsigned long)va_arg(args, void *), 16, field_width, precision, flags);
			break;

		case 'x':

			flags |= SMALL;

		case 'X':

			if (qualifier == 'l')
				str = number(str, va_arg(args, unsigned long), 16, field_width, precision, flags);
			else
				str = number(str, va_arg(args, unsigned int), 16, field_width, precision, flags);
			break;

		case 'd':
		case 'i':

			flags |= SIGN;
		case 'u':

			if (qualifier == 'l')
				str = number(str, va_arg(args, unsigned long), 10, field_width, precision, flags);
			else
				str = number(str, va_arg(args, unsigned int), 10, field_width, precision, flags);
			break;

		case 'n':

			if (qualifier == 'l')
			{
				long *ip = va_arg(args, long *);
				*ip = (str - buf);
			}
			else
			{
				int *ip = va_arg(args, int *);
				*ip = (str - buf);
			}
			break;

		case '%':

			*str++ = '%';
			break;

		default:

			*str++ = '%';
			if (*fmt)
				*str++ = *fmt;
			else
				fmt--;
			break;
		}
	}
	*str = '\0';
	return str - buf;
}

/*
	FRcolor： 前景色/字体颜色
	BKcolor: 背景色
	fmt    : 格式化字符串
*/

int color_printk(unsigned int FRcolor, unsigned int BKcolor, const char *fmt, ...)
{
	int i = 0;
	int count = 0;
	int line = 0;
	va_list args;
	va_start(args, fmt);

	// 解析格式
	i = vsprintf(buf, fmt, args);

	va_end(args);

	for (count = 0; count < i || line; count++)
	{
		////	add \n \b \t
		if (line > 0)
		{
			count--;
			goto Label_tab;
		}
		if ((unsigned char)*(buf + count) == '\n')
		{
			Pos.YPosition++;
			Pos.XPosition = 0;
		}
		else if ((unsigned char)*(buf + count) == '\b')
		{
			Pos.XPosition--;
			if (Pos.XPosition < 0)
			{
				// 这里逻辑是退行 如果y也小于0则是最后一行
				Pos.XPosition = (Pos.XResolution / Pos.XCharSize - 1) * Pos.XCharSize;
				Pos.YPosition--;
				if (Pos.YPosition < 0)
					Pos.YPosition = (Pos.YResolution / Pos.YCharSize - 1) * Pos.YCharSize;
			}
			putchar(Pos.FB_addr, Pos.XResolution, Pos.XPosition * Pos.XCharSize, Pos.YPosition * Pos.YCharSize, FRcolor, BKcolor, ' ');
		}
		else if ((unsigned char)*(buf + count) == '\t')
		{
			line = ((Pos.XPosition + 8) & ~(8 - 1)) - Pos.XPosition;

		Label_tab:
			line--;
			putchar(Pos.FB_addr, Pos.XResolution, Pos.XPosition * Pos.XCharSize, Pos.YPosition * Pos.YCharSize, FRcolor, BKcolor, ' ');
			Pos.XPosition++;
		}
		else
		{
			putchar(Pos.FB_addr, Pos.XResolution, Pos.XPosition * Pos.XCharSize, Pos.YPosition * Pos.YCharSize, FRcolor, BKcolor, (unsigned char)*(buf + count));
			Pos.XPosition++;
		}

		if (Pos.XPosition >= (Pos.XResolution / Pos.XCharSize))
		{
			Pos.YPosition++;
			Pos.XPosition = 0;
		}
		if (Pos.YPosition >= (Pos.YResolution / Pos.YCharSize))
		{
			Pos.YPosition = 0;
		}
	}
	return i;
}

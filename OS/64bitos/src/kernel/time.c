#include "time.h"
#include "lib.h"

// 因为70端口bit7是nmi中断使能 所以这边时钟0x80或
#define CMOS_READ(addr) ({      \
    io_out8(0x70, 0x80 | addr); \
    io_in8(0x71);               \
})

// BCD编码 也就是4bit为一个单元表示一个十进制数字 也就是0-9 所以这边读出来的话如果按照16进制显示刚刚好 比如2023年 读出来就是0x23 0x20
// 所以这边乘以的是0x100 后面有有转换函数
int get_cmos_time(struct time *time)
{
    // cli();
    do
    {
        time->year = CMOS_READ(0x09) + CMOS_READ(0x32) * 0x100;
        time->month = CMOS_READ(0x08);
        time->day = CMOS_READ(0x07);
        time->hour = CMOS_READ(0x04);
        time->minute = CMOS_READ(0x02);
        time->second = CMOS_READ(0x00);

    } while (time->second != CMOS_READ(0x00));
    io_out8(0x70, 0x00);
    // sti();
}
#ifndef __TIME_H__
#define __TIME_H__
struct time
{
    int second; // index 00
    int minute; // 02
    int hour;   // 04
    int day;    // 07
    int month;  // 08
    int year;   // 32+09
};

int get_cmos_time(struct time *time);
#endif
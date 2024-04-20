// gcc -m32 -fno-stack-protector -no-pie fmtdemo.c -o ../target/fmtdemo -g
// gcc -fno-stack-protector -no-pie fmtdemo.c -o ../target/fmtdemo64 -g

#include <stdio.h>

void main()
{
    char fmt[128];
    int arg1 =1,arg2= 0x88888888,arg3=-1;

    char arg4[10]="ABCD";
    scanf("%s",fmt);
    printf(fmt,arg1,arg2,arg3,arg4);
    printf("\n");
}
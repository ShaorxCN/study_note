// gcc -m32 -fno-stack-protector -no-pie lib_test.c -o ../target/test -g
#include <stdio.h>

void main()
{
    char str[1024];
    while(1)
    {
        memset(str,'\0',1024);
        read(0,str,1024);
        printf(str);
        fflush(stdout);
    }
}
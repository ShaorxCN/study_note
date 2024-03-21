#include <stdio.h>
#include <string.h>


/*gcc -fno-stack-protector -no-pie no_stack_canary.c  -o no_stack_canary*/
void success(){puts("success!");}

void vulnerable()
{
    char s[12];
    gets(s);
    puts(s);
    return;
}


void main(int argc,char **argv)
{
    vulnerable();
    return;
}
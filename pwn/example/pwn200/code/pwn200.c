// gcc -m32 -fstack-protector -no-pie pwn200.c -o ../target/pwn200  -g
// gcc -m32 -fstack-protector -no-pie -static pwn200.c -o ../target/pwn2002  -g
#include <stdio.h>
#include <stdlib.h>


void canary_protect_me(void)
{
    system("/bin/sh");
}


int main(void)
{
    setvbuf(stdout,0ll,2,0ll);
    setvbuf(stdin,0ll,1,0ll);
    char buf[40];
    gets(buf);
    printf(buf);
    gets(buf);
    return 0;
}
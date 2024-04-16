// gcc -g -m32 -fno-stack-protector -z execstack overflow.c -o ../target/a.out
// r `python -c 'print("A"*24+"B"*4+"C"*233)'`
#include <stdio.h>
#include <string.h>

void validate_passwd(char *passwd)
{
    char passwd_buf[11];
    unsigned char passwd_len = strlen(passwd);

    if(passwd_len>=4 && passwd_len <=8)
    {
        printf("good %d\n",passwd_len);
        printf("%c\n",*passwd);
        strcpy(passwd_buf,passwd);
    }
    else
    {
        printf("bad\n");
    }
}

int main(int argc,char *argv[])
{
    validate_passwd(argv[1]);
    return 0;
}
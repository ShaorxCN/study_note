#include <unistd.h>


// gcc -m32 -fno-stack-protector -z noexecstack -no-pie dep.c -o ../target/nopie.out
void vuln_func()
{
    char buf[100];
    read(STDIN_FILENO,buf,256);
}

int main(int argc,char **argv)
{
    vuln_func();
    write(STDOUT_FILENO,"Hello world!\n",13);
}
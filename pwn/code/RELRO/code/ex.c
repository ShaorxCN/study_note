#include <stdio.h>
#include <stdlib.h>

// gcc -g -z execstack -no-pie -z norelro ex.c -o ../target/norelro
// gcc -g -z execstack -no-pie -z lazy ex.c -o ../target/relro_lazy
// gcc -g    -Wl,-z,relro,-z,now -o ../target/relro_full  ex.c  
int main(int argc ,char **argv)
{
    size_t *p = (size_t *)strtol(argv[1],NULL,16);
    p[0] = 0x41414141;
    printf("RELRO: %x\n",(unsigned int)*p);
    return 0;
}
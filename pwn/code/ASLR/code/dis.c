#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>


// gcc -no-pie  dis.c -ldl  关闭pie 以及指定动态链接器ld.so  还需要/proc/sys/kernel/randomize_va_space 0设置关闭aslr
// 此时地址输出都不会变 
// 默认aslr是2 也就是说stack heap以及system libc都会会变化 只有程序本身不会随机化
// gcc dis.c -ldl 则是全部变化 （默认pie)
int main()
{
    int stack;
    int *heap = malloc(sizeof(int));
    void *handle = dlopen("libc.so.6",RTLD_NOW|RTLD_GLOBAL);

    printf("executable: %p\n",&main);
    printf("system@plt: %p\n",&system);
    printf("heap: %p\n",heap);
    printf("stack: %p\n",&stack);
    printf("libc: %p\n",handle);
    free(heap);
    return 0;
}
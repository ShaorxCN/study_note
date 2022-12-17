#include <stdio.h>

int test()
{
    int i=0;
    i = i+2;
    printf("the function name is %s",__FUNCTION__);// c99 是__func__
    return i;
}

int main()
{
    test();
    return 0;
}

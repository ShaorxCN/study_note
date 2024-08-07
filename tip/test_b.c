# include <stdio.h>


/**
 1.满足当前结构体字节数模该成员自身有效对齐数为0 则ok 不然填充直到满足。
 2.结构体字节数模成员最大对齐字节数为0 则ok 不然填充直到满足(即结构体大小为其最大成员的整数倍)

此处char 1B 当前结构体0  0%1 = 0 满足 当前结构体字节数为0+1 = 1
int为4 当前1  1%4 = 1 不满足 直到加到4满足 当前结构体字节数为4+4 = 8
char 为1  8%1 = 0 当前结构体字节数为8+1=9
不满足2 填充为12 满足
 */
struct STUDENT
{
    char a;
    int b;
    char c;
}data;


/**
    同理 char 1 0%1=0 满足 结构体1
         char 1 1%1 = 0 满足 结构体2
         int 4  4%2=0 满足 结构体6
         不满足2 填充 8 满足
 */
struct STUDENT2
{
    char a;
    char b;
    int c;
}data2;


int main(void)
{
    printf("%p, %p, %p\n", &data.a, &data.b, &data.c);  
    printf("%d\n", sizeof(data)); // 12

    printf("%p, %p, %p\n", &data2.a, &data2.b, &data2.c);  
    printf("%d\n", sizeof(data2)); // 8
    return 0;
}
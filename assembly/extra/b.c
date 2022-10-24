// 还是参数入栈
// 第一个参数知道参数的个数
void showchar(int, int, ...);
main()
{
    showchar(2, 2, 'a', 'b');
    printf("%x\n", main);
}

void showchar(int n, int color, ...)
{
    int a;
    for (a = 0; a != n; a++)
    {

        *(char far *)(0xb8000000 + 160 * 10 + 80 + a + a) = *(int *)(_BP + 8 + a + a);
        *(char far *)(0xb8000000 + 160 * 10 + 81 + a + a) = color;
    }
}
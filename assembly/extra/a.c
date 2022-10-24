// 参数先入栈 后面的参数先入栈
// call 的子函数中 bp入栈
// Sp 复制给 bp 获取栈顶 这时候最上面是bp
// 通过bp+2n的形式获取参数 跳过bp (栈是字 也就是双字节 每次sp操作数是2) 函数返回后再pop

void showchar(char a, int b);
main()
{
    showchar('a', 2);
    printf("%x\n", main);
}

void showchar(char a, int b)
{
    *(char far *)(0xb8000000 + 160 * 10 + 80) = a;
    *(char far *)(0xb8000000 + 160 * 10 + 81) = b;
}
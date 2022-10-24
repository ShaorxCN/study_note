// 无法返回 报错 tc无法link 再c0s中找不到symbol _main 但是masm的link可以
f()
{
    *(char far *)(0xb8000000 + 160 * 10 + 80) = 'a';
    *(char far *)(0xb8000000 + 160 * 10 + 81) = 2;
}
void f(void);
main()
{
    _AX = 1;
    _BX = 1;
    _CX = 2;

    f();
    printf("%x\n", main);
}

void f(void)
{
    _AX = _BX + _CX;
}
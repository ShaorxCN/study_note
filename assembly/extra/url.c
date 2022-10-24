main()
{
    _AX = 1;
    _BX = 1;
    _CX = 2;
    _AX = _BX + _CX; // 先mov ax, bx 再 add ax, cx _AH = _BL + _CL;
    _AL = _BH + _CH;
    printf("%x\n", main);
}

int func(int arg1,int arg2,int arg3,int arg4,int arg5,int arg6,int arg7,int arg8)
{
    int loc1 = arg1+1;
    int loc2 = arg8+8;

    return loc1+loc2;
}


int main()
{
    return func(0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88);
}

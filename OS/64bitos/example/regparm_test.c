int q = 0x5a;
int t1 = 1;
int t2 = 2;
int t3 = 3;
int t4 = 4;


#define REGPARAM0 __attribute((regparm(0)))
#define REGPARAM3 __attribute__((regparm(3)))


void REGPARAM0 p1(int a)
{
    q = a+1;
}



void REGPARAM3 p2(int a,int b,int c,int d)
{
    q = a+b+c+d;
}

int main()
{
    p1(t1);
    p2(t1,t2,t3,t4);
    return 0;
}


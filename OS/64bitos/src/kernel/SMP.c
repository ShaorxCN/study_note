void SMP_init()
{
    int i;
    unsigned int a, b, c, d;
    for (i = 0;; i++)
    {
        get_cpuid(0xb, i, &a, &b, &c, &d);
        if ((c >> 8 & 0xff) == 0)
            break;
        color_printk(WHITE, BLACK, "local APIC ID Package_../Core_2/SMT_1,type(%x) Width:%#010x,num of logical processor(%x)\n", c >> 8 & 0xff, a & 0x1f, b & 0xff);
    }
    color_printk(WHITE, BLACK, "x2APIC ID level:%#010x\tx2APIC ID the current logical processor:%#010x\n", c & 0xff, d);
}
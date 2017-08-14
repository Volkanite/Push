#include <push.h>


static int CalcSpeed_nv50(int base_freq, int m1, int m2, int n1, int n2, int p)
{
    return (int)((float)(n1*n2) / (m1*m2) * base_freq) >> p;
}

float GetClock_nv50(int base_freq, unsigned int pll, unsigned int pll2)
{
    int m1, m2, n1, n2, p;

    p = (pll >> 16) & 0x03;
    m1 = pll2 & 0xFF;
    n1 = (pll2 >> 8) & 0xFF;

    /* This is always 1 for NV5x? */
    m2 = 1;
    n2 = 1;

    /* The clocks need to be multiplied by 4 for some reason. Is this 4 stored in 0x4000/0x4004? */
    return (float)4 * CalcSpeed_nv50(base_freq, m1, m2, n1, n2, p) / 1000;
}


float nv50_get_core_clock()
{
    int pll = ReadGpuRegister(0x4028);
    int pll2 = ReadGpuRegister(0x402c);
    int base_freq = 25000;

    return (float)GetClock_nv50(base_freq, pll, pll2);
}


float nv50_get_memory_clock()
{
    int pll = ReadGpuRegister(0x4008);
    int pll2 = ReadGpuRegister(0x400c);
    int base_freq = 27000;

    return (float)GetClock_nv50(base_freq, pll, pll2);
}
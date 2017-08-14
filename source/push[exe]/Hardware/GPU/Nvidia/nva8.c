#include <push.h>


int nva8_get_core_clock()
{
    UINT32 P = 0, N = 0, M = 0;
    UINT32 coef;

    coef = ReadGpuRegister(0x4204);

    M = (coef & 0x000000ff) >> 0;
    N = (coef & 0x0000ff00) >> 8;
    P = (coef & 0x003f0000) >> 16;

    int freq = 405000 * N / (M * P);

    return freq / 1000;
}


int nva8_get_memory_clock()
{
    UINT32 P = 0, N = 0, M = 0;
    UINT32 coef;

    coef = ReadGpuRegister(0x4004);

    M = (coef & 0x000000ff) >> 0;
    N = (coef & 0x0000ff00) >> 8;
    P = (coef & 0x003f0000) >> 16;

    int freq = 405000 * N / (M * P);

    return freq / 1000;
}
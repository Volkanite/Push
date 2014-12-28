#include <Windows.h>
#include <stdio.h>


HMODULE 
GetD3DCompiler()
{
    WCHAR buf[32];
    int i;
    HMODULE mod;

    for (i = 50; i >= 30; i--)
    {
        swprintf_s(
            buf, 
            ARRAYSIZE(buf), 
            L"D3DCompiler_%d.dll", 
            i
            );

        mod = LoadLibraryExW(
            buf, 
            NULL, 
            NULL
            );
        
        if (mod)
            return mod;
    }

    return NULL;
}
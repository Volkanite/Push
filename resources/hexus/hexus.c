#include <Windows.h>
#include <stdio.h>
#include "hexus.h"


#define MAX_JMP_SZ 14
#define NOP 0x90 // opcode for NOP
#define JMP 0xE9 // opcode for JUMP

//static const size_t relativeJmpSize = 1 + sizeof(DWORD);
//static const size_t absoluteJmpSize = 2 + sizeof(DWORD)+sizeof(DWORD_PTR);
#define relativeJmpSize 5 // the size of JMP <address>
#define absoluteJmpSize 14 // the size of JMP <address>
typedef enum AddressingMode
{
    Relative = relativeJmpSize,
    Absolute = absoluteJmpSize
}AddressingMode;
#if !defined(_M_AMD64) && !defined(_M_IX86)
#error "_M_AMD64 or _M_IX86 must be defined"
#endif

#ifdef _M_IX86
#include "./hde/hde32.h"
typedef hde32s HDE;
#define HDE_DISASM(code, hs) hde32_disasm(code, hs)
#else
#include "./hde/hde64.h"
typedef hde64s HDE;
#define HDE_DISASM(code, hs) hde64_disasm(code, hs)
#endif


DWORD64 GetDetourLength(const LPVOID lpStart, AddressingMode jmpType)
{
    size_t totalLen = 0;
    LPBYTE lpbDataPos = (LPBYTE)lpStart;

    while (totalLen < jmpType)
    {
        HDE hs;

        size_t len = HDE_DISASM((LPVOID)lpbDataPos, &hs);
        lpbDataPos += len;
        totalLen += len;
    }

    return totalLen;
}


void WriteJump(const LPBYTE lpbFrom, const LPBYTE lpbTo, AddressingMode jmpType)
{
    if (jmpType == Absolute)
    {
        lpbFrom[0] = 0xFF;
        lpbFrom[1] = 0x25;

#ifdef _M_IX86
        // FF 25 [ptr_to_jmp(4bytes)][jmp(4bytes)]
        *(PDWORD)(lpbFrom + 2) = (DWORD)lpbFrom+6;
        *(PDWORD)(lpbFrom + 6) = (DWORD)lpbTo;
#else
        // FF 25 [ptr_to_jmp(4bytes)][jmp(8bytes)]
        *(PDWORD)(lpbFrom + 2) = 0;
        *(PDWORD_PTR)(lpbFrom + 6) = (DWORD_PTR)lpbTo;
#endif
    }

    else if (jmpType == Relative)
    {
        // E9 [to - from - jmp_size]
        lpbFrom[0] = 0xE9;
        DWORD offset = (DWORD)lpbTo-(DWORD)lpbFrom-relativeJmpSize;
        *(PDWORD)(lpbFrom + 1) = (DWORD)offset;
    }
}


AddressingMode GetAddressingMode(const LPBYTE lpbFrom, const LPBYTE lpbTo)
{
    const DWORD_PTR upper = (DWORD_PTR)max(lpbFrom, lpbTo);
    const DWORD_PTR lower = (DWORD_PTR)min(lpbFrom, lpbTo);

    return (upper - lower > 0x7FFFFFFF) ? Absolute : Relative;
}


LPVOID RecurseJumps(LPVOID lpAddr)
{
    LPBYTE lpbAddr = (LPBYTE)lpAddr;

    // Absolute
    if (*lpbAddr == 0xFF && *(lpbAddr + 1) == 0x25)
    {
        LPVOID lpDest = NULL;

#ifdef _M_IX86
        lpDest = **(LPVOID**)(lpbAddr + 2);
#else
        if (*(DWORD*)(lpbAddr + 2) != 0)
        {
            lpDest = *(LPVOID**)(lpbAddr + *(PDWORD)(lpbAddr + 2) + 6);
        }
        else
        {
            lpDest = *(LPVOID**)(lpbAddr + 6);
        }
#endif

        return RecurseJumps(lpDest);
    }

    // Relative Near
    else if (*lpbAddr == 0xE9)
    {
        LPVOID lpDest = NULL;

#ifdef _M_IX86
        lpDest = (LPVOID)(*(PDWORD)(lpbAddr + 1) + (DWORD)lpbAddr+relativeJmpSize);
#else
        lpDest = (LPVOID)(*(PDWORD)(lpbAddr + 1) + (DWORD64)(lpbAddr)+relativeJmpSize);
        DWORD64 basefixed = ((DWORD64)lpbAddr & 0xFFFFFFFF00000000);
        DWORD disp = (DWORD)lpDest;
        lpDest = (LPVOID)(basefixed + disp);
#endif

        return RecurseJumps(lpDest);
    }

    // Relative Short
    else if (*lpbAddr == 0xEB)
    {
        BYTE offset = *(lpbAddr + 1);

        // Jmp forwards
        if (offset > 0x00 && offset <= 0x7F)
        {
            LPVOID lpDest = (LPVOID)(lpbAddr + 2 + offset);
            return RecurseJumps(lpDest);
        }

        // Jmp backwards
        else if (offset > 0x80 && offset <= 0xFF) // tbh none should be > FD
        {
            offset = -abs(offset);
            LPVOID lpDest = (LPVOID)(lpbAddr + 2 - offset);
            return RecurseJumps(lpDest);
        }
    }

    return lpAddr;
}


void* DetourCreate( LPVOID FunctionOrig,  LPVOID FunctionDetour, DETOUR_PROPERTIES *Properties )
{
    BYTE *orig, *hook;
    DWORD dwProt = 0;
    AddressingMode origJmpType, trampJmpType;
    int len = 0;

    orig = RecurseJumps(FunctionOrig);;
    hook = FunctionDetour;

    // Determine which jump type is necessary from orig to hook
    origJmpType = GetAddressingMode(orig, hook);

    len = GetDetourLength(orig, origJmpType);

    //Next we allocate a place in memory for the bytes we are going to overwrite in the original, plus the size of a jump <address> instruction:

    BYTE *tramp = HeapAlloc(GetProcessHeap(), 0, len + MAX_JMP_SZ);

    // Determine which jump types are necessary
    trampJmpType = GetAddressingMode(tramp, orig);

    //Next we want to copy the bytes of original + length to the allocated memory place :
    memcpy(tramp, orig, len);
    
     //Next we want to insert a jump back to the original + length at the end of those intructions we just copied over :
    
    //tramp += len; // increment to the end of the copied bytes
    WriteJump(tramp + len, orig + len, trampJmpType); // Write a jump to the orig from the tramp
    
    // Trim the tramp.
    DWORD trampSize;
    trampSize = (DWORD)(len + trampJmpType);
    /*m_trampoline.resize(dwTrampSize);*/
    
    // Enable full access for when tramp is executed
    if (VirtualProtect(tramp, trampSize, PAGE_EXECUTE_READWRITE, &dwProt) == FALSE) { return 0; }
    
    // Write jump from orig to detour

        if (VirtualProtect(orig, len, PAGE_EXECUTE_READWRITE, &dwProt) == FALSE) { return 0; }

        ///For good practice we want to NOP (x86 no operation) out all the bytes at the original that we have saved to the memory allocated place
        memset(orig, NOP, len);

        WriteJump(orig, hook, origJmpType);

        ///Next we want to put back the old protection flags :
        VirtualProtect(orig, len, dwProt, &dwProt);

    // Flush cache to make sure CPU doesn't execute old instructions
    FlushInstructionCache(GetCurrentProcess(), orig, len);

    Properties->Origin = orig;
    Properties->Trampoline = tramp;
    Properties->Length = len;

    // And finally we want to return a pointer to the start of the trampoline
    return tramp;
}


BOOL DetourDestroy( DETOUR_PROPERTIES* Properties )
{
    DWORD dwProt = 0;

    VirtualProtectEx(GetCurrentProcess(), Properties->Origin, Properties->Length, PAGE_EXECUTE_READWRITE, &dwProt);

    memcpy(Properties->Origin, Properties->Trampoline, Properties->Length);
    VirtualProtect(Properties->Origin, Properties->Length, dwProt, &dwProt);

    HeapFree(GetProcessHeap(), 0, Properties->Trampoline);
    Properties->Length = 0;
    return TRUE;
}

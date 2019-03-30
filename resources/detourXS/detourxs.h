/*
    DetourXS v2 by Dominic Tobias
    Revision 1 (29/03/2011): Fixed potential bug with relative near recursion in 64bit
    Revision 2 (30/03/2011): Couple of semantic changes, added FlushInstructionCache
    dominictobias.com
*/

#ifndef __DETOURXS_H
#define __DETOURXS_H

#include <Windows.h>
#include <vector>
#include <algorithm>

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

class DetourXS
{
public:
    static const size_t relativeJmpSize = 1 + sizeof(DWORD);
    static const size_t absoluteJmpSize = 2 + sizeof(DWORD) + sizeof(DWORD_PTR);
    static const size_t absoluteCallSize = 10;

    enum AddressingMode : size_t
    {
        Relative = relativeJmpSize,
        Absolute = absoluteJmpSize
    };

public:
    DetourXS();
    DetourXS(LPVOID lpFuncOrig, LPVOID lpFuncDetour);
    ~DetourXS();

    BOOL Create(const LPVOID lpFuncOrig, const LPVOID lpFuncDetour);
    BOOL Destroy();

private:
    LPVOID RecurseJumps(LPVOID lpAddr);
    size_t GetDetourLenAuto(const LPVOID lpStart, AddressingMode addressingMode);
    void WriteJump(const LPBYTE lpbFrom, const LPBYTE lpbTo);
    void WriteJump(const LPBYTE lpbFrom, const LPBYTE lpbTo, AddressingMode jmpType);
    void WriteCall(const LPBYTE lpbFrom, const LPBYTE lpbTo);
    void WriteCall(const LPBYTE lpbFrom, const LPBYTE lpbTo, AddressingMode callType);
    void WriteOpCode(const LPBYTE lpbFrom, const LPBYTE lpbTo, AddressingMode addressingMode, BYTE opCode);
    AddressingMode GetAddressingMode(const LPBYTE lpbFrom, const LPBYTE lpbTo);
    BOOLEAN ContainsRelativeCall(const LPBYTE lpbStart, INT DetourLength);
    LPVOID GetRelativeCallAddress(const LPBYTE lpbStart, INT DetourLength);
    int SearchRelativeCall(const LPBYTE lpbStart, INT DetourLength);
    LPVOID ChainedTo(LPVOID lpFunc);
    
public:
    LPVOID GetTrampoline()
    {
        return m_trampoline.empty() ? nullptr : m_trampoline.data();
    }

    size_t GetDetourLen()
    {
        return m_detourLen;
    }

    BOOL Created()
    {
        return m_Created;
    }

    // Use only if you know what you're doing. You are not guaranteed the function address is
    // where the detour will actually take place if you are chaining detours or the function 
    // starts with a jump. Use empty constructor and call before Create.
    void SetDetourLen(size_t len)
    {
        m_detourLen = len;
    }

private:
    LPVOID m_lpFuncOrig;
    LPVOID m_lpFuncDetour;
    LPBYTE m_lpbFuncOrig;
    LPBYTE m_lpbFuncDetour;
    AddressingMode m_OrigJmp;
    AddressingMode m_TrampJmp;
    std::vector<BYTE> m_trampoline;
    size_t m_detourLen;
    BOOL m_Created;
};

#endif // __DETOURXS_H
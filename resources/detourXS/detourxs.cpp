#include "detourxs.h"


DetourXS::DetourXS()
{
    m_detourLen = 0;
    m_Created = FALSE;
}


DetourXS::DetourXS(LPVOID lpFuncOrig, LPVOID lpFuncDetour)
{
    m_detourLen = 0;
    Create(lpFuncOrig, lpFuncDetour);
}


DetourXS::~DetourXS()
{
    Destroy();
}


BOOL DetourXS::Create(const LPVOID lpFuncOrig, const LPVOID lpFuncDetour)
{
    DWORD dwProt;
    DWORD dwTrampSize = 0;
    DWORD bytesCopied;
    BOOLEAN bContainsRelativeCall = FALSE;
    LPBYTE addrToWriteJmp;

    // Already created, need to Destroy() first
    if(m_Created == TRUE || lpFuncOrig == NULL)
    {
        return FALSE;
    }

    // Init
    m_lpFuncOrig = RecurseJumps(lpFuncOrig);
    m_lpFuncDetour = lpFuncDetour;
    m_lpbFuncOrig = reinterpret_cast<LPBYTE>(m_lpFuncOrig);
    m_lpbFuncDetour = reinterpret_cast<LPBYTE>(m_lpFuncDetour);
    m_trampoline.resize(50, 0x00); // data() must not be relocated to determine jmp type

    // Determine which jump is necessary
    m_OrigJmp = GetAddressingMode(m_lpbFuncOrig, m_lpbFuncDetour);
    m_TrampJmp = GetAddressingMode(m_trampoline.data(), m_lpbFuncOrig);

    // Determine detour length
    if(m_detourLen == 0 && (m_detourLen = GetDetourLenAuto(m_lpFuncOrig, m_OrigJmp)) == 0)
    {
        return FALSE;
    }

    // Copy orig bytes to trampoline.

    if (ContainsRelativeCall(m_lpbFuncOrig, m_detourLen))
    {
        LPBYTE relativeCallInstructionAddress = (LPBYTE) GetRelativeCallAddress(m_lpbFuncOrig, m_detourLen);
        int relativeCallAddress = *(int*)(relativeCallInstructionAddress + 1);
        LPBYTE callAddr = (relativeCallInstructionAddress + 5) + relativeCallAddress;
        LPBYTE addressOfCallInstructionInTrampoline;
        LPBYTE endOfDetour;
        LPBYTE nextInstructionAfterCall;
        AddressingMode callType;
        int offset;
        
        offset = relativeCallInstructionAddress - m_lpbFuncOrig;
        addressOfCallInstructionInTrampoline = m_trampoline.data() + offset;
        callType = GetAddressingMode(addressOfCallInstructionInTrampoline, callAddr);
        endOfDetour = m_lpbFuncOrig + m_detourLen;
        nextInstructionAfterCall = relativeCallInstructionAddress + 5;

        std::copy(m_lpbFuncOrig, relativeCallInstructionAddress, m_trampoline.begin());
        WriteCall(addressOfCallInstructionInTrampoline, callAddr, callType);
        std::copy(nextInstructionAfterCall, endOfDetour, addressOfCallInstructionInTrampoline + 6);

        bContainsRelativeCall = TRUE;
        bytesCopied = endOfDetour - nextInstructionAfterCall;
        addrToWriteJmp = m_trampoline.data() + m_detourLen + 1;
    }
    else
    {
        std::copy(m_lpbFuncOrig, m_lpbFuncOrig + m_detourLen, m_trampoline.begin());
    }
    
    if (bContainsRelativeCall)
    {
        // Write a jump to the orig from the tramp
        WriteJump(m_trampoline.data() + m_detourLen + 1, m_lpbFuncOrig + m_detourLen, m_TrampJmp);
    }
    else
    {
        // Write a jump to the orig from the tramp
        WriteJump(m_trampoline.data() + m_detourLen, m_lpbFuncOrig + m_detourLen, m_TrampJmp);
    }

    // Trim the tramp.
    dwTrampSize = m_detourLen + m_TrampJmp;
    if (bContainsRelativeCall) dwTrampSize += 8 + 1;
    m_trampoline.resize(dwTrampSize);

    // Enable full access for when tramp is executed
    if(VirtualProtect(m_trampoline.data(), m_trampoline.size(), PAGE_EXECUTE_READWRITE, &dwProt) == FALSE) { return FALSE; }

    // Write jump from orig to detour
    if(VirtualProtect(m_lpFuncOrig, m_detourLen, PAGE_EXECUTE_READWRITE, &dwProt) == FALSE) { return FALSE; }
    memset(m_lpFuncOrig, 0x90, m_detourLen);
    WriteJump(m_lpbFuncOrig, m_lpbFuncDetour);
    VirtualProtect(m_lpFuncOrig, m_detourLen, dwProt, &dwProt);

    // Flush cache to make sure CPU doesn't execute old instructions
    FlushInstructionCache(GetCurrentProcess(), m_lpFuncOrig, m_detourLen);

    m_Created = TRUE;
    return TRUE;
}


BOOL DetourXS::Destroy()
{
    DWORD dwProt;
    VirtualProtect(m_lpFuncOrig, m_detourLen, PAGE_EXECUTE_READWRITE, &dwProt);
    memcpy(m_lpFuncOrig, m_trampoline.data(), m_detourLen);
    VirtualProtect(m_lpFuncOrig, m_detourLen, dwProt, &dwProt);

    m_trampoline.clear();
    m_detourLen = 0;
    m_Created = FALSE;
    return TRUE;
}


LPVOID DetourXS::RecurseJumps(LPVOID lpAddr)
{
    LPBYTE lpbAddr = reinterpret_cast<LPBYTE>(lpAddr);

    // Absolute
    if(*lpbAddr == 0xFF && *(lpbAddr + 1) == 0x25)
    {
        LPVOID lpDest = nullptr;

        #ifdef _M_IX86
            lpDest = **reinterpret_cast<LPVOID**>( lpbAddr + 2 );
        #else
        if(*reinterpret_cast<DWORD*>(lpbAddr + 2) != 0)
        {
            lpDest = *reinterpret_cast<LPVOID**>( lpbAddr + *reinterpret_cast<PDWORD>(lpbAddr + 2) + 6 );
        }
        else
        {
            lpDest = *reinterpret_cast<LPVOID**>( lpbAddr + 6 );
        }
        #endif

        return RecurseJumps(lpDest);
    }

    // Relative Near
    else if(*lpbAddr == 0xE9)
    {
        LPVOID lpDest = nullptr;

    #ifdef _M_IX86
        lpDest = reinterpret_cast<LPVOID>( *reinterpret_cast<PDWORD>(lpbAddr + 1) + reinterpret_cast<DWORD>(lpbAddr) + relativeJmpSize );
    #else
        lpDest = reinterpret_cast<LPVOID>( *reinterpret_cast<PDWORD>(lpbAddr + 1) + reinterpret_cast<DWORD>(lpbAddr) + relativeJmpSize 
            + (reinterpret_cast<DWORD_PTR>(GetModuleHandle(NULL)) & 0xFFFFFFFF00000000) );
    #endif

        return RecurseJumps(lpDest);
    }

    // Relative Short
    else if(*lpbAddr == 0xEB)
    {
        BYTE offset = *(lpbAddr + 1);

        // Jmp forwards
        if(offset > 0x00 && offset <= 0x7F)
        {
            LPVOID lpDest = reinterpret_cast<LPVOID>( lpbAddr + 2 + offset );
            return RecurseJumps(lpDest);
        }

        // Jmp backwards
        else if(offset > 0x80 && offset <= 0xFF) // tbh none should be > FD
        {
            offset = -abs(offset);
            LPVOID lpDest = reinterpret_cast<LPVOID>( lpbAddr + 2 - offset );
            return RecurseJumps(lpDest);
        }
    }

    return lpAddr;
}


size_t DetourXS::GetDetourLenAuto(const LPVOID lpStart, AddressingMode jmpType)
{
    size_t totalLen = 0;
    LPBYTE lpbDataPos = reinterpret_cast<LPBYTE>(lpStart);

    while(totalLen < jmpType)
    {
        size_t len = LDE(reinterpret_cast<LPVOID>(lpbDataPos), 0);
        lpbDataPos += len;
        totalLen += len;
    }

    return totalLen;
}


void DetourXS::WriteJump(const LPBYTE lpbFrom, const LPBYTE lpbTo)
{
    AddressingMode jmpType = GetAddressingMode(lpbFrom, lpbTo);
    WriteJump(lpbFrom, lpbTo, jmpType);
}


void DetourXS::WriteJump(const LPBYTE lpbFrom, const LPBYTE lpbTo, AddressingMode jmpType)
{
    if (jmpType == Absolute)
    {
        lpbFrom[0] = 0xFF;
        lpbFrom[1] = 0x25;

#ifdef _M_IX86
        // FF 25 [ptr_to_jmp(4bytes)][jmp(4bytes)]
        *reinterpret_cast<PDWORD>(lpbFrom + 2) = reinterpret_cast<DWORD>(lpbFrom)+6;
        *reinterpret_cast<PDWORD>(lpbFrom + 6) = reinterpret_cast<DWORD>(lpbTo);
#else
        // FF 25 [ptr_to_jmp(4bytes)][jmp(8bytes)]
        *reinterpret_cast<PDWORD>(lpbFrom + 2) = 0;
        *reinterpret_cast<PDWORD_PTR>(lpbFrom + 6) = reinterpret_cast<DWORD_PTR>(lpbTo);
#endif
    }

    else if (jmpType == Relative)
    {
        // E9 [to - from - jmp_size]
        lpbFrom[0] = 0xE9;
        DWORD offset = reinterpret_cast<DWORD>(lpbTo)-reinterpret_cast<DWORD>(lpbFrom)-relativeJmpSize;
        *reinterpret_cast<PDWORD>(lpbFrom + 1) = static_cast<DWORD>(offset);
    }
}


void DetourXS::WriteCall(const LPBYTE lpbFrom, const LPBYTE lpbTo)
{
    AddressingMode callType = GetAddressingMode(lpbFrom, lpbTo);
    WriteCall(lpbFrom, lpbTo, callType);
}


void DetourXS::WriteCall(const LPBYTE lpbFrom, const LPBYTE lpbTo, AddressingMode callType)
{
    LPBYTE offset = (LPBYTE)GetTrampoline() + m_detourLen + m_TrampJmp + 1;
    LPBYTE stfu = reinterpret_cast<LPBYTE>(offset - (lpbFrom+6));

    if (callType == Absolute)
    {
        lpbFrom[0] = 0xFF;
        lpbFrom[1] = 0x15;

#ifdef _M_IX86
        // FF 15 [ptr_to_call(4bytes)][call(4bytes)]
        *reinterpret_cast<PDWORD>(lpbFrom + 2) = reinterpret_cast<DWORD>(lpbFrom)+6;
        *reinterpret_cast<PDWORD>(lpbFrom + 6) = reinterpret_cast<DWORD>(lpbTo);
#else
        // FF 15 [ptr_to_call(4bytes)][call(8bytes)]
        *reinterpret_cast<PDWORD>(lpbFrom + 2) = reinterpret_cast<DWORD>(stfu);
        *reinterpret_cast<PDWORD_PTR>(offset) = reinterpret_cast<DWORD_PTR>(lpbTo);
#endif
    }

    else if (callType == Relative)
    {
        // EB [to - from - call_size]
        lpbFrom[0] = 0xE9;
        DWORD offset = reinterpret_cast<DWORD>(lpbTo)-reinterpret_cast<DWORD>(lpbFrom)-relativeJmpSize;
        *reinterpret_cast<PDWORD>(lpbFrom + 1) = static_cast<DWORD>(offset);
    }
}


DetourXS::AddressingMode DetourXS::GetAddressingMode(const LPBYTE lpbFrom, const LPBYTE lpbTo)
{
    const DWORD_PTR upper = reinterpret_cast<DWORD_PTR>((std::max)(lpbFrom, lpbTo));
    const DWORD_PTR lower = reinterpret_cast<DWORD_PTR>((std::min)(lpbFrom, lpbTo));

    return (upper - lower > 0x7FFFFFFF) ? Absolute : Relative;
}


BOOLEAN DetourXS::ContainsRelativeCall(const LPBYTE lpbStart, INT DetourLength)
{
    #ifdef _M_IX86
    return FALSE;
    #else

    int i;

    for (i = 0; i < DetourLength; i++)
    {
        if (lpbStart[i] == 0xE8) //call 0x0000
        {
            return TRUE;
        }
    }

    return FALSE;
    #endif
}


LPVOID DetourXS::GetRelativeCallAddress(const LPBYTE lpbStart, INT DetourLength)
{
    #ifdef _M_IX86
    return NULL;
    #else

    int i;

    for (i = 0; i < DetourLength; i++)
    {
        if (lpbStart[i] == 0xE8) //call 0x0000
        {
            return (lpbStart + i);
        }
    }

    return NULL;
    #endif
}
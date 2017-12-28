#include <Windows.h>
//#include <stdio.h>
#include "wow64.h"


__declspec(naked) void* memcpy64(unsigned long long Dst, unsigned long long Src, unsigned long len)
{
    __asm
    {
        push ebp
            mov ebp, esp
            push esi
            push edi
            X64_Start()
            EMIT(0x67) EMIT(0x48) EMIT(0x8B) EMIT(0x75) EMIT(0x10) //mov rsi, qword ptr[ebp+16]
            EMIT(0x67) EMIT(0x48) EMIT(0x8B) EMIT(0x7D) EMIT(0x08) //mov rdi, qword ptr[ebp+8]
            EMIT(0x67) EMIT(0x8B) EMIT(0x4D) EMIT(0x18) //mov ecx, dword ptr [ebp+24]
            EMIT(0x8a) EMIT(0x06) //mov al, byte ptr[rsi]
            EMIT(0x88) EMIT(0x07) //mov byte ptr[rdi], al
            EMIT(0x48) EMIT(0xFF) EMIT(0xC6) //inc rsi
            EMIT(0x48) EMIT(0xFF) EMIT(0xC7) //inc rdi
            EMIT(0xE2) EMIT(0xF4) //loop e (mov al, byte ptr[rsi])
            X64_End()
            pop edi
            pop esi
            mov esp, ebp
            pop ebp
            ret
    }
}


/*
DWORD64 getTEB64(TEB_64* out)
{
    reg64 reg;
    reg.v = 0;

#ifdef _M_IX86

    _asm
    {
        X64_Start();

        //R12 register should always contain pointer to TEB64 in WoW64 processes
        X64_Push(_R12);

        //below pop will pop QWORD from stack, as we're in x64 mode now
        __asm pop reg.dw[0]

            X64_End();
    }

    memcpy64((DWORD64)out, reg.v, sizeof(TEB_64));

#endif

    return reg.v;
}*/


__declspec(naked) void GetPEB64(PEB64* out)
{
    __asm
    {
        push ebp
            mov ebp, esp
            sub esp, 8
            X64_Start()
            EMIT(0x65) EMIT(0x48) EMIT(0x8B) EMIT(0x04) EMIT(0x25) EMIT(0x60) EMIT(0x00) EMIT(0X00) EMIT(0X00) //mov rax, gs:[0x60]
            EMIT(0x50) //push rax
            EMIT(0x67) EMIT(0x8F) EMIT(0x45) EMIT(0xF8) //pop qword ptr[ebp-8]
            X64_End()
            push 32 //dword
            push[ebp - 0x04] //qword
            push[ebp - 0x08]
            push 0
            push[ebp + 0x08]  //qword
            call memcpy64
            add esp, 20
            mov esp, ebp
            pop ebp
            ret
    }
}




__declspec(naked) int m_memcmp(void* Srcone, void*srcTwo, int len)
{
    __asm
    {
        push esi
            push edi
            push ebx
            mov esi, [esp + 20]
            mov ecx, [esp + 24]
            mov edi, [esp + 16]
            mov ebx, 1
        Compare:
        mov al, byte ptr[edi]
            cmp al, byte ptr[esi]
            jne NotEqual
            inc edi
            inc esi
            dec ecx
            jnz Compare
            mov ebx, 0
        NotEqual:
        mov eax, ebx
            pop ebx
            pop edi
            pop esi
            ret
    }
}


__declspec(naked) unsigned long GetImageBase()
{
    __asm
    {
        mov eax, GetImageBase
            and eax, 0xFFFF0000
        Find:
        cmp word ptr[eax], 0x5A4D
            je End
            sub eax, 0x00010000
            jmp Find
        End :
        ret

    }
}


VOID* Module_GetProcedureAddress(VOID* DllHandle, CHAR* ProcedureName);
VOID* Module_Load(WCHAR* FileName);
INT32 String_Compare(WCHAR* Source, WCHAR* dst);
VOID Memory_Clear(VOID* Region, UINT32 Size);


unsigned long long GetModuleHandle64(wchar_t* ModuleName)
{
    unsigned long long Current = 0;
    unsigned long long ulModule = 0;

    wchar_t szBuffer[512];
    PEB64 Peb64;
    LDR64 Ldr64;
    LDR_DATA_TABLE_ENTRY64 LdrEntry64;

    Memory_Clear(szBuffer, sizeof(szBuffer));
    Memory_Clear(&Peb64, sizeof(PEB64));
    Memory_Clear(&Ldr64, sizeof(LDR64));
    Memory_Clear(&LdrEntry64, sizeof(LDR_DATA_TABLE_ENTRY64));

    GetPEB64(&Peb64);

    if (Peb64.ImageBaseAddress == (unsigned long long)GetModuleHandle(0)/*GetImageBase()*/)
    {
        memcpy64((unsigned long long)&Ldr64, Peb64.Ldr, sizeof(LDR64));
        memcpy64((unsigned long long)&LdrEntry64, Ldr64.InLoadOrderModuleList.Flink, sizeof(LDR_DATA_TABLE_ENTRY64));

        do
        {

            Current = LdrEntry64.InLoadOrderLinks.Flink;

            if (Current == Peb64.Ldr + 0x10)
                break;

            memcpy64((unsigned long long)&LdrEntry64, LdrEntry64.InLoadOrderLinks.Flink, sizeof(LDR_DATA_TABLE_ENTRY64));
            memcpy64((unsigned long long)szBuffer, LdrEntry64.BaseDllName.Buffer, LdrEntry64.BaseDllName.Length);



            if (!m_memcmp(ModuleName, szBuffer, LdrEntry64.BaseDllName.Length))
            {
                ulModule = LdrEntry64.DllBase;
                break;
            }
        } while (1);
    }
    return ulModule;
}


unsigned long long GetNextArgument(unsigned long * ulCurrentArg, unsigned long ulArgCount, unsigned long long *pulArgs)
{

    unsigned long long NextArg = 0;
    if (*ulCurrentArg < ulArgCount)
    {
        NextArg = pulArgs[(*ulCurrentArg)++];
    }
    return NextArg;
}


#pragma warning( push )
#pragma warning( disable : 4409)

unsigned long CallFunction64(
    unsigned long long ulAddress,
    unsigned long ulOrdinal,
    unsigned long long * pulArgs,
    unsigned long ulArgCount,
    unsigned char ucType
    )
{

    unsigned long ulNtStatus = 0;
    unsigned long long StackArguments = 0;
    unsigned long long StackCount = 0;
    unsigned long ulCurrentArg = 0;



    unsigned long long Arg1 = GetNextArgument(&ulCurrentArg, ulArgCount, pulArgs);
    unsigned long long Arg2 = GetNextArgument(&ulCurrentArg, ulArgCount, pulArgs);
    unsigned long long Arg3 = GetNextArgument(&ulCurrentArg, ulArgCount, pulArgs);
    unsigned long long Arg4 = GetNextArgument(&ulCurrentArg, ulArgCount, pulArgs);



    if (ulArgCount > 4)
    {
        StackArguments = (unsigned long long)&pulArgs[3];
        StackCount = ulArgCount - 4;
    }




    __asm
    {
        push edi
            push esi
            X64_Start()
            push Arg1
            EMIT(0x59) //pop rcx
            push Arg2
            EMIT(0x5A) //pop rdx
            push Arg3
            EMIT(0x41) EMIT(0x58) //pop r8
            push Arg4
            EMIT(0x41) EMIT(0x59) //pop r9
            push StackArguments
            EMIT(0x5F) //pop rdi
            push StackCount
            EMIT(0x5E) //pop rsi
            test esi, esi
            jz Procedure
        LoadStack :
        push[edi + (esi * 8)]
            sub esi, 1
            jnz LoadStack
        Procedure :
        push ucType
            EMIT(0x58)
            cmp eax, 0
            jne SysCall
            push ulAddress
            EMIT(0x58)
            sub esp, 32
            call eax
            add esp, 32
            jmp StackCleanup
        SysCall :
        push Arg1
            EMIT(0x41) EMIT(0x5A) //pop r10
            mov eax, ulOrdinal
            sub esp, 40
            EMIT(0x0F) EMIT(0x05)  //syscall
            add esp, 40
        StackCleanup:
        push StackCount
            EMIT(0x5E)
            test esi, esi
            jz End
        Cleanup :
        pop edi
            sub esi, 1
            jnz Cleanup
        End :
        mov ulNtStatus, eax
            X64_End()
            pop esi
            pop edi
    }



    return ulNtStatus;



}

#pragma warning( pop )


__declspec(naked) unsigned long strlen64(unsigned long long Dst)
{
    __asm
    {
        push ebp
            mov ebp, esp
            xor ecx, ecx
            X64_Start()
            EMIT(0x67) EMIT(0x48) EMIT(0x8b) EMIT(0x45) EMIT(0x08) //mov rax, qword ptr[ebp+8]
        Calculate:

        EMIT(0x80) EMIT(0x38) EMIT(0x00) //cmp byte ptr[rax], 00
            je Done
            EMIT(0x48) EMIT(0x83) EMIT(0xC0) EMIT(0x01) //add rax, 1
            add ecx, 1
            jmp Calculate

        Done :
        mov eax, ecx
            X64_End()
            mov esp, ebp
            pop ebp
            ret
    }
}


typedef int(*TYPE_memcmp)(const void *Buf1, const void *Buf2, int Size);
extern TYPE_memcmp      ntdll_memcmp;
typedef int(*TYPE_strcmp)(const char *Str1, const char *Str2);
extern TYPE_strcmp      ntdll_strcmp;
typedef int(*TYPE_strncmp)(const char *Str1, const char *Str2, int MaxCount);
extern TYPE_strncmp     ntdll_strncmp;
typedef UINT32(*TYPE_strlen)(const char *Str);
extern TYPE_strlen      ntdll_strlen;


unsigned long long GetProcAddress64(unsigned long long ulModule, char *szFunction)
{

    unsigned long long ulFunction = 0;

    IMAGE_DOS_HEADER DosHeader;
    IMAGE_NT_HEADERS64 NtHeaders;

    Memory_Clear(&DosHeader, sizeof(IMAGE_DOS_HEADER));
    Memory_Clear(&NtHeaders, sizeof(IMAGE_NT_HEADERS64));

    memcpy64((unsigned long long)&DosHeader, ulModule, sizeof(IMAGE_DOS_HEADER));
    memcpy64((unsigned long long)&NtHeaders, ulModule + DosHeader.e_lfanew, sizeof(IMAGE_NT_HEADERS64));

    if (NtHeaders.Signature == IMAGE_NT_SIGNATURE)
    {
        char szName[256];
        IMAGE_EXPORT_DIRECTORY Exports;
        unsigned long ulVA;
        unsigned long ulSize;
        unsigned long i;

        Memory_Clear(szName, sizeof(szName));
        Memory_Clear(&Exports, sizeof(IMAGE_EXPORT_DIRECTORY));

        ulVA = NtHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
        ulSize = NtHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
        i = 0;

        memcpy64((unsigned long long)&Exports, ulModule + ulVA, sizeof(IMAGE_EXPORT_DIRECTORY));

        for (i; i < Exports.NumberOfNames; i++)
        {
            unsigned long long ulNameOffset = 0;
            memcpy64((unsigned long long)&ulNameOffset, Exports.AddressOfNames + ulModule + (i * 4), sizeof(unsigned long));
            ulNameOffset += ulModule;
            memcpy64((unsigned long long)szName, ulNameOffset, strlen64(ulNameOffset));

            if (!ntdll_memcmp(szFunction, szName, ntdll_strlen(szFunction)))
            {
                unsigned long long ulAddressOffset = 0;
                unsigned long long ulOrdinalOffset = 0;
                memcpy64((unsigned long long)&ulOrdinalOffset, (Exports.AddressOfNameOrdinals + ulModule + (i * 2)), sizeof(unsigned short));
                memcpy64((unsigned long long)&ulAddressOffset, (Exports.AddressOfFunctions + ulModule + (ulOrdinalOffset * 4)), sizeof(unsigned long));
                if (ulAddressOffset > ulVA && ulAddressOffset < (ulVA + ulSize))
                {
                    ulFunction = 0;
                    break;
                }
                ulFunction = (ulAddressOffset + ulModule);
                break;
            }
        }

    }

    return ulFunction;
}


#pragma warning( push )
#pragma warning( disable : 4409)
#pragma warning( disable : 4102)
unsigned long SysCall64(unsigned long ulOrdinal, unsigned long long * pulArgs, unsigned long ulArgCount)
{

    unsigned long ulNtStatus = 0;
    unsigned long long StackArguments = 0;
    unsigned long long StackCount = 0;
    unsigned long ulCurrentArg = 0;



    unsigned long long Arg1 = GetNextArgument(&ulCurrentArg, ulArgCount, pulArgs);
    unsigned long long Arg2 = GetNextArgument(&ulCurrentArg, ulArgCount, pulArgs);
    unsigned long long Arg3 = GetNextArgument(&ulCurrentArg, ulArgCount, pulArgs);
    unsigned long long Arg4 = GetNextArgument(&ulCurrentArg, ulArgCount, pulArgs);



    if (ulArgCount > 4)
    {
        StackArguments = (unsigned long long)&pulArgs[3];
        StackCount = ulArgCount - 4;
    }


    __asm
    {
        push edi
            push esi
            X64_Start()

            push Arg1
            EMIT(0x59) //pop rcx
            push Arg2
            EMIT(0x5A) //pop rdx
            push Arg3
            EMIT(0x41) EMIT(0x58) //pop r8
            push Arg4
            EMIT(0x41) EMIT(0x59) //pop r9


            push StackArguments
            EMIT(0x5F) //pop rdi
            push StackCount
            EMIT(0x5E) //pop rsi

            sub esp, 40 //Stack frame spilling registers

            mov eax, esi
            imul eax, 8
            sub esp, eax //Rest of arguments

            test esi, esi
            jz SysCall

        LoadStack :
        EMIT(0x67) EMIT(0x48) EMIT(0x8B) EMIT(0x0C) EMIT(0xF7) //mov rcx, qword ptr[edi + (esi*8)]
            EMIT(0x67) EMIT(0x48) EMIT(0x89) EMIT(0x4C) EMIT(0xF4) EMIT(0x20) //mov qword ptr [esp+(esi*8)+32], rcx
            sub esi, 1
            jnz LoadStack

        SysCall :
        push Arg1
            EMIT(0x41) EMIT(0x5A) //pop r10
            mov eax, ulOrdinal
            EMIT(0x0F) EMIT(0x05)  //syscall

            push StackCount
            EMIT(0x5E) //pop rsi

            mov ulNtStatus, eax

            mov eax, esi
            imul eax, 8

            add esp, eax
            add esp, 40
        End:


        X64_End()
            pop esi
            pop edi
    }


    return ulNtStatus;



}
#pragma warning( pop )


enum CreateThreadFlags
{
    NoThreadFlags = 0x0000,
    CreateSuspended = 0x0001,
    NoDllCallbacks = 0x0002,
    HideFromDebug = 0x0004,
};


DWORD64 GetRemoteModuleHandle(HANDLE ProcessHandle, WCHAR* ModuleName);
DWORD64 GetRemoteProcAddress(HANDLE ProcessHandle, DWORD64 BaseAddress, const char* name_ord);

typedef unsigned long long  DWORD64;
VOID Inject64stub(HANDLE ProcessHandle, VOID* pLibRemote, DWORD64 LoadLibraryEntry);

/*typedef struct _PROCESS_BASIC_INFORMATION
{
NTSTATUS ExitStatus;
PEB* PebBaseAddress;
ULONG_PTR AffinityMask;
KPRIORITY BasePriority;
HANDLE UniqueProcessId;
HANDLE InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;*/
typedef struct _PROCESS_BASIC_INFORMATION_64
{
    NTSTATUS ExitStatus;
    ULONG    Reserved0;
    UINT64        PebBaseAddress;
    UINT64        AffinityMask;
    LONG     BasePriority;
    ULONG    Reserved1;
    UINT64        uUniqueProcessId;
    UINT64        uInheritedFromUniqueProcessId;
}PROCESS_BASIC_INFORMATION_64;


// NtWow64QueryInformationProcess64
typedef NTSTATUS(__stdcall *TYPE_NtWow64QueryInformationProcess64)
(
    HANDLE ProcessHandle,
    ULONG  ProcessInformationClass,
    VOID*  ProcessInformation64,
    ULONG  Length,
    ULONG* ReturnLength
);
typedef DWORD64 PTR64;
// NtWow64ReadVirtualMemory64
typedef NTSTATUS(__stdcall *TYPE_NtWow64ReadVirtualMemory64)(
    HANDLE   ProcessHandle,
    PTR64  BaseAddress,
    VOID*   Buffer,
    UINT64  BufferLength,
    UINT64* ReturnLength
    );

TYPE_NtWow64QueryInformationProcess64  NtWow64QueryInformationProcess64;
TYPE_NtWow64ReadVirtualMemory64        NtWow64ReadVirtualMemory64;
typedef unsigned long long  DWORD64;
//template <typename T, typename NGF, int A>

typedef struct _UNICODE_STRING_64
{
    WORD Length;
    WORD MaximumLength;
    DWORD64 Buffer;
}UNICODE_STRING_64;

typedef struct _LIST_ENTRY_64
{
    DWORD64 Flink;
    DWORD64 Blink;
}LIST_ENTRY_64;

typedef struct _PEB_64
{
    union
    {
        struct
        {
            BYTE InheritedAddressSpace;
            BYTE ReadImageFileExecOptions;
            BYTE BeingDebugged;
            BYTE BitField;
        };
        DWORD64 dummy01;
    };
    DWORD64 Mutant;
    DWORD64 ImageBaseAddress;
    DWORD64 Ldr;
    DWORD64 ProcessParameters;
    DWORD64 SubSystemData;
    DWORD64 ProcessHeap;
    DWORD64 FastPebLock;
    DWORD64 AtlThunkSListPtr;
    DWORD64 IFEOKey;
    DWORD64 CrossProcessFlags;
    DWORD64 UserSharedInfoPtr;
    DWORD SystemReserved;
    DWORD AtlThunkSListPtr32;
    DWORD64 ApiSetMap;
    DWORD64 TlsExpansionCounter;
    DWORD64 TlsBitmap;
    DWORD TlsBitmapBits[2];
    DWORD64 ReadOnlySharedMemoryBase;
    DWORD64 HotpatchInformation;
    DWORD64 ReadOnlyStaticServerData;
    DWORD64 AnsiCodePageData;
    DWORD64 OemCodePageData;
    DWORD64 UnicodeCaseTableData;
    DWORD NumberOfProcessors;
    union
    {
        DWORD NtGlobalFlag;
        DWORD dummy02;
    };
    LARGE_INTEGER CriticalSectionTimeout;
    DWORD64 HeapSegmentReserve;
    DWORD64 HeapSegmentCommit;
    DWORD64 HeapDeCommitTotalFreeThreshold;
    DWORD64 HeapDeCommitFreeBlockThreshold;
    DWORD NumberOfHeaps;
    DWORD MaximumNumberOfHeaps;
    DWORD64 ProcessHeaps;
    DWORD64 GdiSharedHandleTable;
    DWORD64 ProcessStarterHelper;
    DWORD64 GdiDCAttributeList;
    DWORD64 LoaderLock;
    DWORD OSMajorVersion;
    DWORD OSMinorVersion;
    WORD OSBuildNumber;
    WORD OSCSDVersion;
    DWORD OSPlatformId;
    DWORD ImageSubsystem;
    DWORD ImageSubsystemMajorVersion;
    DWORD64 ImageSubsystemMinorVersion;
    DWORD64 ActiveProcessAffinityMask;
    DWORD64 GdiHandleBuffer[30];
    DWORD64 PostProcessInitRoutine;
    DWORD64 TlsExpansionBitmap;
    DWORD TlsExpansionBitmapBits[32];
    DWORD64 SessionId;
    ULARGE_INTEGER AppCompatFlags;
    ULARGE_INTEGER AppCompatFlagsUser;
    DWORD64 pShimData;
    DWORD64 AppCompatInfo;
    UNICODE_STRING_64 CSDVersion;
    DWORD64 ActivationContextData;
    DWORD64 ProcessAssemblyStorageMap;
    DWORD64 SystemDefaultActivationContextData;
    DWORD64 SystemAssemblyStorageMap;
    DWORD64 MinimumStackCommit;
    DWORD64 FlsCallback;
    LIST_ENTRY_64 FlsListHead;
    DWORD64 FlsBitmap;
    DWORD FlsBitmapBits[4];
    DWORD64 FlsHighIndex;
    DWORD64 WerRegistrationData;
    DWORD64 WerShipAssertPtr;
    DWORD64 pContextData;
    DWORD64 pImageHeaderHash;
    DWORD64 TracingFlags;
    DWORD64 CsrServerReadOnlySharedMemoryBase;
}PEB_64;


typedef enum _PROCESSINFOCLASS
{
    ProcessBasicInformation, // 0, q: PROCESS_BASIC_INFORMATION, PROCESS_EXTENDED_BASIC_INFORMATION
    ProcessQuotaLimits
}PROCESSINFOCLASS;
#define STATUS_SUCCESS                  ((NTSTATUS)0x00000000L)
NTSTATUS __stdcall NtDelayExecution(
    BOOLEAN Alertable,
    LARGE_INTEGER* DelayInterval
    );
NTSleep(UINT32 Milliseconds)
{
    LARGE_INTEGER interval;

    interval.QuadPart = Milliseconds * -10000LL;
    NtDelayExecution(FALSE, &interval);
}


VOID* GetRemoteProcessEnvironmentBlock(HANDLE ProcessHandle, PEB_64* ProcessEnvironmentBlock)
{
    PROCESS_BASIC_INFORMATION_64 info;
    ULONG bytes = 0;
    NTSTATUS status;

    Memory_Clear(&info, sizeof(PROCESS_BASIC_INFORMATION_64));

    NtWow64QueryInformationProcess64 = (TYPE_NtWow64QueryInformationProcess64)Module_GetProcedureAddress(
        Module_Load(L"ntdll.dll"),
        "NtWow64QueryInformationProcess64"
        );

    status = NtWow64QueryInformationProcess64(ProcessHandle, ProcessBasicInformation, &info, sizeof(info), &bytes);

    NtWow64ReadVirtualMemory64 = (TYPE_NtWow64ReadVirtualMemory64)Module_GetProcedureAddress(
        Module_Load(L"ntdll.dll"),
        "NtWow64ReadVirtualMemory64"
        );

    ProcessEnvironmentBlock->Ldr = 0;

    //sometimes it takes a while for Ldr to be filled.
    while (ProcessEnvironmentBlock->Ldr == 0)
    {
        status = NtWow64ReadVirtualMemory64(ProcessHandle, info.PebBaseAddress, ProcessEnvironmentBlock, sizeof(PEB_64), 0);
        
        NTSleep(500);
    }

    if (bytes > 0 && status == STATUS_SUCCESS)
        return info.PebBaseAddress;

    return 0;
}


typedef struct _PEB_LDR_DATA_64
{
    unsigned long Length;
    unsigned char Initialized;
    DWORD64 SsHandle;
    LIST_ENTRY_64 InLoadOrderModuleList;
    LIST_ENTRY_64 InMemoryOrderModuleList;
    LIST_ENTRY_64 InInitializationOrderModuleList;
    DWORD64 EntryInProgress;
    unsigned char ShutdownInProgress;
    DWORD64 ShutdownThreadId;
}PEB_LDR_DATA_64;
typedef unsigned long long  uint64_t;
typedef uint64_t ptr_t;


typedef struct _LDR_DATA_TABLE_ENTRY_BASE_64
{
    LIST_ENTRY_64 InLoadOrderLinks;
    LIST_ENTRY_64 InMemoryOrderLinks;
    LIST_ENTRY_64 InInitializationOrderLinks;
    DWORD64 DllBase;
    DWORD64 EntryPoint;
    unsigned long SizeOfImage;
    UNICODE_STRING_64 FullDllName;
    UNICODE_STRING_64 BaseDllName;
    unsigned long Flags;
    unsigned short LoadCount;
    unsigned short TlsIndex;
    LIST_ENTRY_64 HashLinks;
    unsigned long TimeDateStamp;
    DWORD64 EntryPointActivationContext;
    DWORD64 PatchInformation;
}LDR_DATA_TABLE_ENTRY_BASE_64;


WCHAR* __stdcall StrStrIW(WCHAR*, WCHAR*);


DWORD64 GetRemoteModuleHandle( HANDLE ProcessHandle, WCHAR* ModuleName )
{
    PEB_64 peb;
    PEB_LDR_DATA_64 ldr;
    NTSTATUS status = 0;

    Memory_Clear(&peb, sizeof(PEB_64));
    Memory_Clear(&ldr, sizeof(PEB_LDR_DATA_64));

    GetRemoteProcessEnvironmentBlock(ProcessHandle, &peb);

    status = NtWow64ReadVirtualMemory64(ProcessHandle, peb.Ldr, &ldr, sizeof(ldr), 0);

    if (status == STATUS_SUCCESS)
    {
        DWORD64 head;

        for (head = ldr.InLoadOrderModuleList.Flink;
            head != (peb.Ldr + FIELD_OFFSET(PEB_LDR_DATA_64, InLoadOrderModuleList));
            NtWow64ReadVirtualMemory64(ProcessHandle, head, &head, sizeof(head), 0))
        {
            wchar_t localPath[260];
            LDR_DATA_TABLE_ENTRY_BASE_64 localdata;

            Memory_Clear(localPath, sizeof(localPath));
            Memory_Clear(&localdata, sizeof(LDR_DATA_TABLE_ENTRY_BASE_64));

            NtWow64ReadVirtualMemory64(ProcessHandle, head, &localdata, sizeof(localdata), 0);
            NtWow64ReadVirtualMemory64(ProcessHandle, localdata.BaseDllName.Buffer, localPath, localdata.FullDllName.Length, 0);

            if (StrStrIW(localPath, ModuleName))
            {
                return localdata.DllBase;
            }
        }
    }

    return 0;
}


#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES    16
#define IMAGE_DOS_SIGNATURE                 0x5A4D      // MZ
#define IMAGE_NT_SIGNATURE                  0x00004550  // PE00
#define IMAGE_NT_OPTIONAL_HDR32_MAGIC      0x10b
#define IMAGE_DIRECTORY_ENTRY_EXPORT          0   // Export Directory

VOID* Memory_Allocate(DWORD Size);


DWORD64 GetRemoteProcAddress(HANDLE ProcessHandle, DWORD64 BaseAddress, const char* name_ord)
{
    IMAGE_DOS_HEADER hdrDos;
    UINT8 hdrNt32[sizeof(IMAGE_NT_HEADERS64)];
    PIMAGE_NT_HEADERS32 phdrNt32 = (PIMAGE_NT_HEADERS32)(hdrNt32);
    PIMAGE_NT_HEADERS64 phdrNt64 = (PIMAGE_NT_HEADERS64)(hdrNt32);
    IMAGE_EXPORT_DIRECTORY* pExpData;
    DWORD expSize = 0;
    size_t expBase = 0;
    WORD *pAddressOfOrds;
    DWORD *pAddressOfNames;
    DWORD *pAddressOfFuncs;
    DWORD i;

    Memory_Clear(&hdrDos, sizeof(IMAGE_DOS_HEADER));
    Memory_Clear(&hdrNt32, sizeof(hdrNt32));

    /// Invalid module
    if (BaseAddress == 0)
    {
        return 0;
    }

    NtWow64ReadVirtualMemory64(ProcessHandle, BaseAddress, &hdrDos, sizeof(hdrDos), 0);

    if (hdrDos.e_magic != IMAGE_DOS_SIGNATURE)
        return 0;

    NtWow64ReadVirtualMemory64(ProcessHandle, BaseAddress + hdrDos.e_lfanew, &hdrNt32, sizeof(IMAGE_NT_HEADERS64), 0);

    if (phdrNt32->Signature != IMAGE_NT_SIGNATURE)
        return 0;

    if (phdrNt32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        expBase = phdrNt32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    else
        expBase = phdrNt64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;

    // Exports are present
    if (expBase)
    {
        if (phdrNt32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            expSize = phdrNt32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
        else
            expSize = phdrNt64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

        pExpData = Memory_Allocate(expSize);

        NtWow64ReadVirtualMemory64(ProcessHandle, BaseAddress + expBase, pExpData, expSize, 0);

        pAddressOfOrds = (WORD*)(
            pExpData->AddressOfNameOrdinals + (size_t)(pExpData)-expBase);

        pAddressOfNames = (DWORD*)(
            pExpData->AddressOfNames + (size_t)(pExpData)-expBase);

        pAddressOfFuncs = (DWORD*)(
            pExpData->AddressOfFunctions + (size_t)(pExpData)-expBase);

        for (i = 0; i < pExpData->NumberOfFunctions; ++i)
        {
            WORD OrdIndex = 0xFFFF;
            char *pName = NULL;

            // Find by index
            if ((size_t)(name_ord) <= 0xFFFF)
            {
                OrdIndex = (WORD)(i);
            }
            // Find by name
            else if ((size_t)(name_ord) > 0xFFFF && i < pExpData->NumberOfNames)
            {
                pName = (char*)(pAddressOfNames[i] + (size_t)(pExpData)-expBase);
                OrdIndex = (WORD)(pAddressOfOrds[i]);
            }
            else
                return 0;

            if (((size_t)(name_ord) <= 0xFFFF && (WORD)((uintptr_t)name_ord) == (OrdIndex + pExpData->Base)) ||
                ((size_t)(name_ord) > 0xFFFF && ntdll_strcmp(pName, name_ord) == 0))
            {
                DWORD64 procAddress = pAddressOfFuncs[OrdIndex] + BaseAddress;

                return procAddress;

                break;
            }
        }
    }

    return 0;
}


DWORD64 NtCreateThreadEx64( HANDLE ProcessHandle, DWORD64 StartRoutine, DWORD RemoteMemory )
{
    DWORD64 ulModule = GetModuleHandle64(L"ntdll.dll");
    DWORD64 threadHandle = NULL;

    if (ulModule)
    {
        DWORD64 _NtCreateThreadEx = GetProcAddress64(ulModule, "NtCreateThreadEx");

        if (_NtCreateThreadEx)
        {
            DWORD64 parameters[11];

            Memory_Clear(parameters, sizeof(parameters));

            parameters[0] = (DWORD64)&threadHandle;     // _Out_ PHANDLE ThreadHandle
            parameters[1] = (DWORD64)THREAD_ALL_ACCESS; // _In_ ACCESS_MASK DesiredAccess
            parameters[2] = (DWORD64)NULL;              // _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes
            parameters[3] = (DWORD64)ProcessHandle;     // _In_ HANDLE ProcessHandle
            parameters[4] = (DWORD64)StartRoutine;      // _In_ PVOID StartRoutine
            parameters[5] = (DWORD64)RemoteMemory;      // _In_opt_ PVOID Argument
            parameters[6] = (DWORD64)HideFromDebug;     // _In_ ULONG CreateFlags
            parameters[7] = (DWORD64)0;                 // _In_ SIZE_T ZeroBits
            parameters[8] = (DWORD64)0x1000;            // _In_ SIZE_T StackSize
            parameters[9] = (DWORD64)0x100000;          // _In_ SIZE_T MaximumStackSize
            parameters[10] = (DWORD64)NULL;             // _In_opt_ PPS_ATTRIBUTE_LIST AttributeList

            CallFunction64(_NtCreateThreadEx, 0, parameters, 11, 0);
        }
    }

    return threadHandle;
}

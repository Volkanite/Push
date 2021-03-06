#ifndef HEAVENS_GATE_H_
#define HEAVENS_GATE_H_

#define EMIT(a) __asm __emit (a)

#define X64_Start_with_CS(_cs) \
                { \
        EMIT(0x6A) EMIT(_cs)                         /*  push   _cs             */ \
        EMIT(0xE8) EMIT(0) EMIT(0) EMIT(0) EMIT(0)   /*  call   $+5             */ \
        EMIT(0x83) EMIT(4) EMIT(0x24) EMIT(5)        /*  add    dword [esp], 5  */ \
        EMIT(0xCB)                                   /*  retf                   */ \
                }

#define X64_End_with_CS(_cs) \
                { \
        EMIT(0xE8) EMIT(0) EMIT(0) EMIT(0) EMIT(0)                                 /*  call   $+5                   */ \
        EMIT(0xC7) EMIT(0x44) EMIT(0x24) EMIT(4) EMIT(_cs) EMIT(0) EMIT(0) EMIT(0) /*  mov    dword [rsp + 4], _cs  */ \
        EMIT(0x83) EMIT(4) EMIT(0x24) EMIT(0xD)                                    /*  add    dword [rsp], 0xD      */ \
        EMIT(0xCB)                                                                 /*  retf                         */ \
                }

#define X64_Start() X64_Start_with_CS(0x33)
#define X64_End() X64_End_with_CS(0x23)



typedef struct _LDR64
{
    unsigned long Length;
    unsigned long Initialized;
    unsigned long long ssHandle;
    LIST_ENTRY64 InLoadOrderModuleList;
    LIST_ENTRY64 InMemoryOrderModuleList;
    LIST_ENTRY64 InInitializationOrderModuleList;
}LDR64;

typedef struct _PEB64
{
    unsigned char InheritedAddressSpace;
    unsigned char ReadImageFileExecOptions;
    unsigned char BeingDebugged;
    unsigned long ImageUsesLargePages : 1;
    unsigned long IsProtectedProcess : 1;
    unsigned long IsLegacyProcess : 1;
    unsigned long IsImageDynamicallyRelocated : 1;
    unsigned long SpareBits : 4;
    unsigned long long Mutant;
    unsigned long long ImageBaseAddress;
    unsigned long long Ldr;
}PEB64;


typedef struct _LSA_UNICODE_STRING64 {
    USHORT Length;
    USHORT MaximumLength;
    DWORD64 Buffer;
} UNICODE_STRING64, *PUNICODE_STRING64;


typedef struct _LDR_DATA_TABLE_ENTRY64
{
    LIST_ENTRY64 InLoadOrderLinks;
    LIST_ENTRY64 InMemoryOrderLinks;
    LIST_ENTRY64 InInitializationOrderLinks;
    DWORD64 DllBase;
    DWORD64 EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING64 FullDllName;
    UNICODE_STRING64 BaseDllName;
    ULONG Flags;
    WORD LoadCount;
} LDR_DATA_TABLE_ENTRY64;


#endif
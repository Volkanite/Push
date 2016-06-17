#ifndef NTBASIC_H
#define NTBASIC_H


typedef LONG KPRIORITY;

typedef struct _STRING
{
    UINT16 Length;
    UINT16 MaximumLength;
    CHAR* Buffer;
} STRING, *PSTRING, ANSI_STRING, *PANSI_STRING, OEM_STRING, *POEM_STRING;

typedef struct _UNICODE_STRING
{
    UINT16 Length;
    UINT16 MaximumLength;
    WCHAR* Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

// Object attributes

#define OBJ_INHERIT             0x00000002L
#define OBJ_CASE_INSENSITIVE    0x00000040L

typedef struct _OBJECT_ATTRIBUTES {
    DWORD                           Length;
    VOID*                           RootDirectory;
    UNICODE_STRING*                 ObjectName;
    DWORD                           Attributes;
    VOID*                           SecurityDescriptor; // PSECURITY_DESCRIPTOR;
    VOID*    SecurityQualityOfService;
} OBJECT_ATTRIBUTES;

/*#define InitializeObjectAttributes(p, n, a, r, s) { \
    (p)->Length = sizeof(OBJECT_ATTRIBUTES); \
    (p)->RootDirectory = r; \
    (p)->Attributes = a; \
    (p)->ObjectName = n; \
    (p)->SecurityDescriptor = s; \
    (p)->SecurityQualityOfService = NULL; \
    }*/
typedef struct _CLIENT_ID
{
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef struct _CLIENT_ID32
{
    ULONG UniqueProcess;
    ULONG UniqueThread;
} CLIENT_ID32, *PCLIENT_ID32;

typedef struct _CLIENT_ID64
{
    ULONGLONG UniqueProcess;
    ULONGLONG UniqueThread;
} CLIENT_ID64, *PCLIENT_ID64;


#endif //NTBASIC_H

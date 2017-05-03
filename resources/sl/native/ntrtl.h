#ifndef _NTRTL_H
#define _NTRTL_H

typedef struct _RTL_DRIVE_LETTER_CURDIR {

    UINT16 Flags;
    UINT16 Length;
    UINT32 TimeStamp;
    UNICODE_STRING DosPath;

} RTL_DRIVE_LETTER_CURDIR;

typedef struct _RTL_USER_PROCESS_PARAMETERS {

    UINT32 MaximumLength;
    UINT32 Length;
    UINT32 Flags;
    UINT32 DebugFlags;
    VOID *ConsoleHandle;
    UINT32 ConsoleFlags;
    VOID *StdInputHandle;
    VOID *StdOutputHandle;
    VOID *StdErrorHandle;
    UNICODE_STRING CurrentDirectoryPath;
    VOID *CurrentDirectoryHandle;
    UNICODE_STRING DllPath;
    UNICODE_STRING ImagePathName;
    UNICODE_STRING CommandLine;
    VOID *Environment;
    UINT32 StartingPositionLeft;
    UINT32 StartingPositionTop;
    UINT32 Width;
    UINT32 Height;
    UINT32 CharWidth;
    UINT32 CharHeight;
    UINT32 ConsoleTextAttributes;
    UINT32 WindowFlags;
    UINT32 ShowWindowFlags;
    UNICODE_STRING WindowTitle;
    UNICODE_STRING DesktopName;
    UNICODE_STRING ShellInfo;
    UNICODE_STRING RuntimeData;
    RTL_DRIVE_LETTER_CURDIR DLCurrentDirectory[0x20];

} RTL_USER_PROCESS_PARAMETERS;

typedef struct _RTLP_CURDIR_REF
{
    LONG    RefCount;
    VOID*   Handle;
} RTLP_CURDIR_REF;

typedef struct _RTL_RELATIVE_NAME_U
{
    UNICODE_STRING      RelativeName;
    VOID*               ContainingDirectory;
    RTLP_CURDIR_REF*    CurDirRef;
} RTL_RELATIVE_NAME_U;

typedef struct _RTL_BITMAP
{
    UINT32  SizeOfBitMap;
    UINT32* Buffer;
} RTL_BITMAP;


#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS __stdcall RtlStringFromGUID(
    GUID* Guid,
    PUNICODE_STRING GuidString
    );
    
NTSTATUS __stdcall RtlCreateSecurityDescriptor(
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    ULONG Revision
    );
    
BOOLEAN __stdcall RtlCreateUnicodeString(
    UNICODE_STRING  *DestinationString,
    const WCHAR     *SourceString
    );
    
VOID __stdcall RtlFreeUnicodeString( 
    UNICODE_STRING *UnicodeString 
    );
    
LONG __stdcall RtlCompareUnicodeString(
    UNICODE_STRING *String1,
    UNICODE_STRING *String2,
    BOOLEAN CaseInSensitive
    );
    
BOOLEAN __stdcall RtlDosPathNameToNtPathName_U(
    WCHAR*                  DosFileName,
    UNICODE_STRING*         NtFileName,
    WCHAR**                 FilePart,
    RTL_RELATIVE_NAME_U*    RelativeName
    );
    
NTSTATUS __stdcall RtlSetDaclSecurityDescriptor(
    VOID* SecurityDescriptor,
    BOOLEAN DaclPresent,
    ACL* Dacl,
    BOOLEAN DaclDefaulted
    );
    
NTSTATUS __stdcall RtlMakeSelfRelativeSD(
    VOID* AbsoluteSecurityDescriptor,
    VOID* SelfRelativeSecurityDescriptor,
    ULONG* BufferLength
    );
	
NTSTATUS __stdcall RtlUTF8ToUnicodeN(
    WCHAR* UnicodeStringDestination,
	ULONG UnicodeStringMaxByteCount,
	ULONG* UnicodeStringActualByteCount,
    CHAR* UTF8StringSource,
    ULONG UTF8StringByteCount
    );
    
#ifdef __cplusplus
}
#endif

#endif //_NTRTL_H
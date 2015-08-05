#ifndef _FILE_H
#define _FILE_H

typedef VOID(*TYPE_FsProgessRoutine)(
    UINT64 TotalFileSize,
    UINT64 TotalBytesTransferred
    );


class File
{
public:
    static NTSTATUS Create(VOID** FileHandle, WCHAR* FileName, DWORD DesiredAccess, DWORD ShareAccess, DWORD CreateDisposition, DWORD CreateOptions);
    static HANDLE Create( WCHAR* FileName );
    static BOOLEAN Exists(WCHAR *FileName);
    static VOID Copy(WCHAR* SourceFileName, WCHAR* DestinationFileName, TYPE_FsProgessRoutine ProgressRoutine);
    static VOID* Load(WCHAR* FileName, UINT64* FileSize);
    static UINT64 GetSize(WCHAR* FileName);
    static DWORD GetAttributes( WCHAR* FileName );
    static UINT32 Read( HANDLE FileHandle, VOID* Buffer, UINT32 Length );
    static VOID Write( HANDLE FileHandle, VOID* Buffer, UINT32 Length );
    static VOID SetPointer( HANDLE FileHandle, INT64 DistanceToMove );
    static VOID Delete( WCHAR* FileName );
    static VOID Close( HANDLE FileHandle );
};

#endif

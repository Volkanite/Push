#ifndef _FILE_H
#define _FILE_H

typedef VOID(*TYPE_FsProgessRoutine)(
    UINT64 TotalFileSize,
    UINT64 TotalBytesTransferred
    );


NTSTATUS File_Create(VOID** FileHandle, WCHAR* FileName, DWORD DesiredAccess, DWORD ShareAccess, DWORD CreateDisposition, DWORD CreateOptions, DWORD* CreateStatus);
HANDLE File_Open(WCHAR* FileName, DWORD DesiredAccess);
BOOLEAN File_Exists(WCHAR *FileName);
VOID File_Copy(WCHAR* SourceFileName, WCHAR* DestinationFileName, TYPE_FsProgessRoutine ProgressRoutine);
VOID* File_Load(WCHAR* FileName, UINT64* FileSize);
UINT64 File_GetSize(WCHAR* FileName);
BOOLEAN File_GetLastWriteTime(HANDLE FileHandle, FILETIME* LastWriteTime);
DWORD File_GetAttributes(WCHAR* FileName);
UINT32 File_Read(HANDLE FileHandle, VOID* Buffer, UINT32 Length);
VOID File_Write(HANDLE FileHandle, VOID* Buffer, UINT32 Length);
INT64 File_GetPointer(HANDLE FileHandle);
VOID File_SetPointer(HANDLE FileHandle, INT64 DistanceToMove, DWORD MoveMethod);
VOID File_Delete(WCHAR* FileName);
VOID File_Close(HANDLE FileHandle);

#endif

typedef VOID (*TYPE_FsProgessRoutine)(
    UINT64 TotalFileSize,
    UINT64 TotalBytesTransferred
    );
    

#ifdef __cplusplus
extern "C" {
#endif
NTSTATUS SlFileCreate(
    VOID** FileHandle,
    WCHAR* FileName, 
    DWORD DesiredAccess,
    DWORD ShareAccess,
    DWORD CreateDisposition,
    DWORD CreateOptions
    );
    
BOOLEAN SlFileExists( 
    WCHAR *fname 
    );
    
    
/**
* Copies an existing file to a new file.
*
* \param SourceFileName The Win32 file name of source file.
* \param DestinationFileName The Win32 file name for the new
* file.
* \param ProgressRoutine The address of a callback function of
* type TYPE_FsProgessRoutine that is called each time another
* portion of the file has been copied. This parameter can be
* NULL if no progress routine is required.
*/

VOID SlFileCopy(
    WCHAR* SourceFileName,
    WCHAR* DestinationFileName,
    TYPE_FsProgessRoutine ProgressRoutine
    );
    
    
/**
* Loads a file into memory and returns the base address.
*
* \param FileName The Win32 file name.
* \param FileSize Optional, returns the file size.
*/

VOID* SlFileLoad( 
    WCHAR* FileName, 
    UINT64* FileSize 
    );
    
#ifdef __cplusplus
}
#endif

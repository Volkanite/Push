typedef enum{

    SCOPE_CACHE_ONLY,
    SCOPE_ALL_FILES

} FILE_SCOPE;


typedef enum{

    SUCCESS,
    FOLDER_DOES_NOT_EXIST,
    NO_CACHE_FILES_FOUND

} ENUM_FILES_ERROR_CODES;


typedef VOID (*FS_ENUM_DIRECTORY)(
    WCHAR* Directory,
    FILE_DIRECTORY_INFORMATION* Information
    );

typedef VOID (*TYPE_FsProgessRoutine)(
    UINT64 TotalFileSize,
    UINT64 TotalBytesTransferred
    );

VOID FailSafe( WCHAR *pszGame );

VOID MarkForCache( WCHAR *pszFilePath );

VOID UnmarkForCache( WCHAR *pszFilePath, BOOLEAN log );

WCHAR* GetLogFileName( WCHAR *pszPath );

VOID CacheFile(
    WCHAR *pszFilePath,
    CHAR cMountPoint
    );

VOID CacheFiles(CHAR driveLetter);

#ifdef __cplusplus
extern "C" {
#endif

BOOLEAN FolderExists( WCHAR *pszPath );

VOID GetPathOnly( WCHAR *pszFilePath, WCHAR *pszBuffer );


/**
* Enumerates a directory.
*
* \param Directory The Win32 directory name.
* \param SearchPattern Search expression/wildcards.
* \param Callback The address of a callback function of type FS_ENUM_DIRECTORY that is
* called for each file that matches the search expression.
*/
NTSTATUS FsEnumDirectory(
    WCHAR* Directory,
    WCHAR* SearchPattern,
    FS_ENUM_DIRECTORY Callback
    );


/**
* Renames a file.
*
* \param FilePath The Win32 file name.
* \param NewFileName The new file name.
*/
VOID FsRenameFile(
    WCHAR* FilePath,
    WCHAR* NewFileName
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
VOID FsFileCopy(
    WCHAR* SourceFileName,
    WCHAR* DestinationFileName,
    TYPE_FsProgessRoutine ProgressRoutine
    );


/**
* Retrieves the size of a file.
*
* \param FileName The Win32 file name.
*/
UINT64 FsFileGetSize(
    WCHAR* FileName
    );

#ifdef __cplusplus
}
#endif

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
* Retrieves the size of a file.
*
* \param FileName The Win32 file name.
*/
UINT64 FsFileGetSize(
    WCHAR* FileName
    );

/**
* Loads a file into memory and returns the base address.
*
* \param FileName The Win32 file name.
* \param FileSize Optional, returns the file size.
*/
VOID* FsFileLoad( 
    WCHAR* FileName, 
    UINT64* FileSize 
    );


#ifdef __cplusplus
}
#endif

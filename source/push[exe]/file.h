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

BOOLEAN FolderExists( WCHAR *pszPath );

VOID GetPathOnly( WCHAR *pszFilePath, WCHAR *pszBuffer );


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

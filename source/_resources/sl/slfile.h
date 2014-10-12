#ifdef __cplusplus
class SlFileManager{
public:
    VOID CopyFile();
    VOID* CreateFile( WCHAR* FileName );
	VOID CreateShortcut();
    VOID CreateSymbolicLink();
    VOID CompareSymbolicLink();
    VOID DeleteFile();
    VOID DeleteDirectory();
    VOID EnumDirectory();
    BOOLEAN FileExists();
    BOOLEAN FolderExists();
    VOID GetFileAttributes();
    VOID GetFileIndex();
    VOID GetFileSize( WCHAR* FileName );
    VOID GetFileSize( VOID* FileHandle );
	VOID GetLogicalCluster();
	VOID GetLogicalClusters();
	VOID GetNumberOfFragments();
	VOID GetPhysicalSector();
	VOID GetPhysicalSectors();
	VOID* OpenFile( WCHAR* FileName );
    VOID RenameFile();
};

class SlFile
{
    VOID* FileHandle;
public:
    SlFile( WCHAR* FileName );
    ~SlFile();

    VOID Read();
    VOID Write( VOID* Buffer, ULONG Length );
    VOID Write( WCHAR* String );
};
#endif
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
#ifdef __cplusplus
}
#endif
typedef VOID(*SL_ENUM_DIRECTORY)(
    WCHAR* Directory,
    FILE_DIRECTORY_INFORMATION* Information
    );


class Directory
{
public:
    static NTSTATUS Enum( WCHAR* Directory, WCHAR* SearchPattern, SL_ENUM_DIRECTORY Callback );
    static VOID AppendFileName( WCHAR* FileName, WCHAR* Path, WCHAR* Buffer );
};

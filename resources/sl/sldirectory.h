typedef VOID(*SL_ENUM_DIRECTORY)(
    WCHAR* Directory,
    FILE_DIRECTORY_INFORMATION* Information
    );


NTSTATUS Directory_Enum(WCHAR* Directory, WCHAR* SearchPattern, SL_ENUM_DIRECTORY Callback);
VOID Directory_AppendFileName(WCHAR* FileName, WCHAR* Path, WCHAR* Buffer);

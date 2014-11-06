// Define CopyFileEx option flags

#define COPY_FILE_FAIL_IF_EXISTS 0x00000001


typedef
DWORD
(__stdcall *LPPROGRESS_ROUTINE)(
    LARGE_INTEGER TotalFileSize,
    LARGE_INTEGER TotalBytesTransferred,
    LARGE_INTEGER StreamSize,
    LARGE_INTEGER StreamBytesTransferred,
    DWORD dwStreamNumber,
    DWORD dwCallbackReason,
    VOID* hSourceFile,
    VOID* hDestinationFile,
    VOID* lpData
    );
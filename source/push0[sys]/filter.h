extern BOOLEAN FltInitialized;

VOID FltFilterInstall(
    DRIVER_OBJECT* DriverObject
    );

VOID FltStopFiltering(
    );

VOID FltQueueFile(
    WCHAR* File
    );
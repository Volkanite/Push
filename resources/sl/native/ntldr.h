NTSTATUS __stdcall LdrLoadDll(
    WCHAR* SearchPath,
    DWORD* LoadFlags,
    UNICODE_STRING* Name,
    VOID** BaseAddress
    );
typedef struct _NVAPI_MEM_INFO
{
    UINT32 Total;
    UINT32 Free;
} NVAPI_MEM_INFO;


UINT8 GetGeForceTemp(
    );

UINT8 GetGeForceUsage(
    );

VOID GetGeForceMemoryInfo(
    NVAPI_MEM_INFO* MemInfo
    );

typedef struct _PROCESSOR_POWER_INFORMATION
{
    ULONG Number;
    ULONG MaxMhz;
    ULONG CurrentMhz;
    ULONG MhzLimit;
    ULONG MaxIdleState;
    ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;

NTSTATUS __stdcall NtPowerInformation(
    POWER_INFORMATION_LEVEL InformationLevel,
    VOID* InputBuffer,
    ULONG InputBufferLength,
    VOID* OutputBuffer,
    ULONG OutputBufferLength
    );
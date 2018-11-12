#define BOOL INTBOOL

typedef UINT32(__stdcall* PSP_FILE_CALLBACK_W)(
    VOID* Context,
    UINT32 Notification,
    UINT32 Param1,
    UINT32 Param2
    );

typedef struct _SP_DEVINSTALL_PARAMS_W {
    DWORD             cbSize;
    DWORD             Flags;
    DWORD             FlagsEx;
    HANDLE              hwndParent;
    PSP_FILE_CALLBACK_W InstallMsgHandler;
    VOID*             InstallMsgHandlerContext;
    HANDLE          FileQueue;
    ULONG_PTR         ClassInstallReserved;
    DWORD             Reserved;
    WCHAR             DriverPath[260];
} SP_DEVINSTALL_PARAMS_W;


#define LINE_LEN                    256

#pragma pack(push, 1) // exact fit - no padding
typedef struct _SP_DRVINFO_DATA_V2_W {
    DWORD     cbSize;
    DWORD     DriverType;
    ULONG_PTR Reserved;
    WCHAR     Description[LINE_LEN];
    WCHAR     MfgName[LINE_LEN];
    WCHAR     ProviderName[LINE_LEN];
    FILETIME  DriverDate;
    INT64 DriverVersion;
} SP_DRVINFO_DATA_V2_W;
#pragma pack(pop)

HANDLE __stdcall SetupDiGetClassDevsW(
    GUID* ClassGuid,
    WCHAR* Enumerator,
    HANDLE hwndParent,
    DWORD Flags
    );

BOOL __stdcall SetupDiEnumDeviceInfo(
    HANDLE DeviceInfoSet,
    DWORD MemberIndex,
    SP_DEVINFO_DATA* DeviceInfoData
    );

BOOL __stdcall SetupDiGetDeviceRegistryPropertyW(
    HANDLE DeviceInfoSet,
    SP_DEVINFO_DATA* DeviceInfoData,
    DWORD Property,
    DWORD* PropertyRegDataType,
    VOID* PropertyBuffer,
    DWORD PropertyBufferSize,
    DWORD* RequiredSize
    );

BOOL __stdcall SetupDiSetSelectedDevice(
    HANDLE DeviceInfoSet,
    SP_DEVINFO_DATA* DeviceInfoData
    );

BOOL __stdcall SetupDiGetDeviceInstallParamsW(
    HANDLE DeviceInfoSet,
    SP_DEVINFO_DATA* DeviceInfoData,
    SP_DEVINSTALL_PARAMS_W* DeviceInstallParams
    );

BOOL __stdcall SetupDiSetDeviceInstallParamsW(
    HANDLE DeviceInfoSet,
    SP_DEVINFO_DATA* DeviceInfoData,
    SP_DEVINSTALL_PARAMS_W* DeviceInstallParams
    );

BOOL __stdcall SetupDiBuildDriverInfoList(
    HANDLE DeviceInfoSet,
    SP_DEVINFO_DATA* DeviceInfoData,
    DWORD DriverType
    );

BOOL __stdcall SetupDiGetSelectedDriverW(
    HANDLE DeviceInfoSet,
    SP_DEVINFO_DATA* DeviceInfoData,
    SP_DRVINFO_DATA_V2_W* DriverInfoData
    );

BOOL __stdcall SetupDiSetSelectedDriverW(
    HANDLE DeviceInfoSet,
    SP_DEVINFO_DATA* DeviceInfoData,
    SP_DRVINFO_DATA_V2_W* DriverInfoData
    );

BOOL __stdcall SetupDiSelectBestCompatDrv(
    HANDLE DeviceInfoSet,
    SP_DEVINFO_DATA* DeviceInfoData
    );

BOOL __stdcall DiInstallDevice(
    HANDLE             hwndParent,
    HANDLE         DeviceInfoSet,
    SP_DEVINFO_DATA* DeviceInfoData,
    SP_DRVINFO_DATA_V2_W* DriverInfoData,
    DWORD            Flags,
    BOOL*            NeedReboot
    );

#define DIGCF_PRESENT           0x00000002
#define DIGCF_ALLCLASSES        0x00000004

#define SPDRP_DEVICEDESC    (0x00000000)  // DeviceDesc (R/W)
#define SPDRP_HARDWAREID    (0x00000001)  // HardwareID (R/W)
#define SPDRP_COMPATIBLEIDS (0x00000002)  // CompatibleIDs (R/W)

DWORD __stdcall GetLastError();
#define SPDIT_COMPATDRIVER       0x00000002
#pragma pack(push,1)
typedef struct _MOTIONINJOY_CONTROLLER_INF
{
    BYTE Connected;
    BYTE Type;
    BYTE Power;
    BYTE BDAddr[6];
    BYTE Reserved[41];
}MOTIONINJOY_CONTROLLER_INF;

typedef struct _MOTIONINJOY_DEVICE_INF
{

    BYTE    ID;
    BYTE    Version;
    WORD  Revision;
    BYTE    DeviceType;
    BYTE    BDAddr[6];
    BYTE    HCIVersion;
    WORD  HCIRevision;
    BYTE    LMPVersion;
    WORD  Manufacturer;
    WORD  LMPSubVersion;
    BYTE    BTDongleFeature[8];
    BYTE    Reserved[2];
    MOTIONINJOY_CONTROLLER_INF controllers[4];
}MOTIONINJOY_DEVICE_INF;

typedef struct _MOTIONINJOY_COMMON_OPTION
{
    BYTE mode;
    BYTE lVibrationStrength;
    BYTE rVibrationStrength;
    BYTE mouseSpeed_xy;
    BYTE LED;
    BYTE Deadzone_LStick_X;
    BYTE Deadzone_LStick_Y;
    BYTE Deadzone_RStick_X;
    BYTE Deadzone_RStick_Y;
    BYTE Threshold_Triangle;
    BYTE Threshold_Circle;
    BYTE Threshold_Cross;
    BYTE Threshold_Square;
    BYTE Threshold_L2;
    BYTE Threshold_R2;
    BYTE Threshold_L1;
    BYTE Threshold_R1;
    BYTE Threshold_DpadUp;
    BYTE Threshold_DpadRight;
    BYTE Threshold_DpadDown;
    BYTE Threshold_DpadLeft;
    BYTE AutoOff_timeout;
    BYTE mouseWheelSpeed;
}MOTIONINJOY_COMMON_OPTION;

typedef struct _MOTIONINJOY_INPUT_OPTION
{
    WORD Auto;
    WORD Trubo;
    WORD Duration;
    WORD Interval;
    BYTE Macro[16];
    WORD Maping[48];
}MOTIONINJOY_INPUT_OPTION;

typedef struct _MOTIONINJOY_APP_OPTION
{
    BYTE Dummy[8];
    MOTIONINJOY_COMMON_OPTION CommonOption;
    BYTE Dummy2[77];
    MOTIONINJOY_INPUT_OPTION InputOption;
}MOTIONINJOY_APP_OPTION;

typedef struct MOTIONINJOY_BUTTON_MACRO
{
    BYTE DeviceIndex;
    BYTE EIndex;
    BYTE MacroIndex;
    BYTE ButtonIndex;
    UINT32 Duration;
    WORD Command[6];
    BYTE Button[128];
    BYTE Dummy[108];
}MOTIONINJOY_BUTTON_MACRO;

typedef struct MOTIONINJOY_MACRO
{
    // Duration of the macro. Standard is 800ms
    UINT32 Duration;

    // One word for each button, each word holds which button shoud be pressed
    WORD Command[6];

    // Intervals for each button
    BYTE ButtonIntervals[128][6];
}MOTIONINJOY_MACRO;
#pragma pack(pop)

typedef enum _MOTIONINJOY_MODE
{
    Keyboard = 0,
    DirectInput = 4,
    XInput = 5
}_MOTIONINJOY_MODE;

#define IOCTL_INDEX 0x800
#define FILE_DEVICE_UNKNOWN             0x00000022
#define FILE_WRITE_ACCESS         ( 0x0002 )    // file & pipe
#define METHOD_BUFFERED                 0

#define IOCTL_MIJ_GET_DEVICE_COUNT      CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_INDEX+0, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_MIJ_GET_DEVICE_INF        CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_INDEX+1, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_MIJ_GET_CONFIG_OPTIONS    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_INDEX+2, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_MIJ_BTINQUIRY             CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_INDEX+3, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_MIJ_GET_REMOTE_OPTIONS    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_INDEX+4, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_MIJ_SET_CONFIG_OPTIONS    CTL_CODE(FILE_DEVICE_UNKNOWN, 2148, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_MIJ_SET_CONFIG_MACRO      CTL_CODE(FILE_DEVICE_UNKNOWN, 2149, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_MIJ_VIBRATION             CTL_CODE(FILE_DEVICE_UNKNOWN, 2150, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_MIJ_DISCONNECT            CTL_CODE(FILE_DEVICE_UNKNOWN, 2151, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_MIJ_SET_FEATURE           CTL_CODE(FILE_DEVICE_UNKNOWN, 2152, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_MIJ_SET_REMOTE_OPTIONS    CTL_CODE(FILE_DEVICE_UNKNOWN, 2153, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_MIJ_MOUSE_MOVE            CTL_CODE(FILE_DEVICE_UNKNOWN, 2248u, METHOD_BUFFERED, FILE_WRITE_ACCESS)

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

typedef struct _USB_DEVICES
{
    wchar_t* RegistryId;
    wchar_t* FriendlyName;
}USB_DEVICES;


WCHAR* String_CompareIgnoreCaseN(WCHAR* s1, WCHAR* s2, int n);

VOID Mij_SetProfile(WCHAR* GameName);
VOID Mij_EnumerateDevices();


// DirectInput
// 0x0100 = Axis X+
// 0x0101 = Axis X-
// 0x0102 = Axis Y+
// 0x0103 = Axis Y-
// 0x0106 = Axis RX+
// 0x0107 = Axis RX-
// 0x0108 = Axis RY+
// 0x0109 = Axis RY-
// 0x0200 = Button 1
// 0x0201 = Button 2
// 0x0202 = Button 3
// 0x0203 = Button 4
// 0x0204 = Button 5
// 0x0205 = Button 6
// 0x0206 = Button 7
// 0x0207 = Button 8
// 0x0208 = Button 9
// 0x0209 = Button 10
// 0x020B = Button 12
// 0x020C = Button 13
// 0x0214 = D-Pad Up
// 0x0215 = D-Pad Right
// 0x0216 = D-Pad Down
// 0x0217 = D-Pad Left
// Keyboard
// 0x0302 = A
// 0x0307 = D
// 0x0308 = E
// 0x0311 = N
// 0x0315 = R
// 0x0316 = S
// 0x031A = W
// 0x0329 = Escape
// 0x032B = Tab
// 0x032C = Space
// 0x034D = End
// 0x034F = Arrow Right
// 0x0350 = Arrow Left
// 0x0351 = Arrow Down
// 0x0352 = Arrow Up
// 0x03E1 = Left Shift
// 0x03E4 = Right Ctrl
// Mouse
// 0x0400 = Mouse Move Right
// 0x0401 = Mouse Move Left
// 0x0402 = Mouse Move Down
// 0x0403 = Mouse Move Up
// 0x0404 = Mouse Wheel Up
// 0x0405 = Mouse Wheel Down
// 0x0500 = Mouse Left Button
// 0x0501 = Mouse Right Button
// XInput
// 0x0600 = A Button
// 0x0601 = B Button
// 0x0602 = X Button
// 0x0603 = Y Button
// 0x0604 = Left Bumper
// 0x0605 = Right Bumper
// 0x0606 = Back Button
// 0x0607 = Start Button
// 0x0608 = Left Stick Button
// 0x0609 = Right Stick Button
// 0x060A = XBox Button
// 0x060B = D-Pad Up
// 0x060C = D-Pad Down
// 0x060D = D-Pad Left
// 0x060E = D-Pad Right
// 0x0700 = Left Trigger
// 0x0701 = Right Trigger
// 0x0702 = Left Stick X+
// 0x0703 = Left Stick X-
// 0x0704 = Left Stick Y-
// 0x0705 = Left Stick Y+
// 0x0706 = Right Stick X+
// 0x0707 = Right Stick X-
// 0x0708 = Right Stick Y-
// 0x0709 = Right Stick Y+
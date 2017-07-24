#include <sl.h>


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
    BYTE Dummy2[78];
    MOTIONINJOY_INPUT_OPTION InputOption;
}MOTIONINJOY_APP_OPTION;
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


VOID GetControllerMappingFile(WCHAR* GameName, WCHAR* Buffer)
{
    WCHAR *dot;
    WCHAR batchFile[260];

    String_Copy(batchFile, L"mij\\");
    String_Concatenate(batchFile, GameName);

    dot = String_FindLastChar(batchFile, '.');

    if (dot)
        String_Copy(dot, L".map");
    else
        String_Concatenate(batchFile, L".map");

    String_Copy(Buffer, batchFile);
}


VOID GetSettingsFile(WCHAR* GameName, WCHAR* Buffer)
{
    WCHAR *dot;
    WCHAR batchFile[260];

    String_Copy(batchFile, L"mij\\");
    String_Concatenate(batchFile, GameName);

    dot = String_FindLastChar(batchFile, '.');

    if (dot)
        String_Copy(dot, L".mij");
    else
        String_Concatenate(batchFile, L".mij");

    String_Copy(Buffer, batchFile);
}


VOID SetProfile(WCHAR* GameName)
{
    HANDLE driverHandle;
    IO_STATUS_BLOCK isb;
    MOTIONINJOY_APP_OPTION options;
    wchar_t bigbuff[260];
    void* controllerMapping = NULL;
    wchar_t* settingsFile;

    GetControllerMappingFile(GameName, bigbuff);

    controllerMapping = File_Load(bigbuff, NULL);

    GetSettingsFile(GameName, bigbuff);

    settingsFile = File_Load(bigbuff, NULL);

    // Start our reads after the UTF16-LE character marker
    settingsFile += 1;

    File_Create(
        &driverHandle,
        L"\\\\.\\MIJFilter",
        SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );
    
    Memory_Clear(&options, sizeof(MOTIONINJOY_APP_OPTION));

    if (settingsFile[0] == '0')
        options.CommonOption.mode = Keyboard;
    else if (settingsFile[0] == '4')
        options.CommonOption.mode = DirectInput;
    else if (settingsFile[0] == '5')
        options.CommonOption.mode = XInput;

    options.CommonOption.LED = 129;

    Memory_Copy(options.InputOption.Maping, controllerMapping, 96);
    NtDeviceIoControlFile(driverHandle, NULL, NULL, NULL, &isb, IOCTL_MIJ_SET_CONFIG_OPTIONS, &options, 256, NULL, 0);
    File_Close(driverHandle);
}
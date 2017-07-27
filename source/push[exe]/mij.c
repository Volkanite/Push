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

    // Intervals for each button?
    BYTE Button_1[128]; 
    BYTE Button_2[128];
    BYTE Button_3[128];
    BYTE Button_4[128];
    BYTE Button_5[128];
    BYTE Button_6[128];
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
VOID SetMacro(UINT8 Count, MOTIONINJOY_MACRO* Macro);


WORD SomeMacro[6] = {
    0x0214, //D-Pad Up
    0x0216, //D-Pad Down
    0x0215, //D-Pad Right
    0x0217, //D-Pad Left
    0x0215, //D-Pad Right
    0x0204  //Button 5
};

// 0x0204 = Button 5
// 0x0214 = D-Pad Up
// 0x0215 = D-Pad Right
// 0x0216 = D-Pad Down
// 0x0217 = D-Pad Left


VOID SetProfile( WCHAR* GameName )
{
    HANDLE driverHandle;
    IO_STATUS_BLOCK isb;
    MOTIONINJOY_APP_OPTION options;
    wchar_t bigbuff[260];
    void* controllerMapping = NULL;
    wchar_t* settingsFile = NULL;
    MOTIONINJOY_MACRO macro;
    BOOLEAN setTestMacro = TRUE;

    Memory_Clear(&macro, sizeof(MOTIONINJOY_MACRO));

    macro.Duration = 800; //800 milli-seconds
    Memory_Copy(macro.Command, SomeMacro, sizeof(macro.Command));

    macro.Button_1[10] = 0x01;
    macro.Button_2[10] = 0x02;
    macro.Button_3[10] = 0x04;
    macro.Button_4[10] = 0x08;
    macro.Button_5[10] = 0x10;
    macro.Button_6[10] = 0x20;

    if (setTestMacro)
        SetMacro(1, &macro);

    GetControllerMappingFile(GameName, bigbuff);

    controllerMapping = File_Load(bigbuff, NULL);

    GetSettingsFile(GameName, bigbuff);

    settingsFile = File_Load(bigbuff, NULL);

    if (!settingsFile)
        return;

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
    options.CommonOption.AutoOff_timeout = 132;
    options.CommonOption.Deadzone_LStick_X = 10;
    options.CommonOption.Deadzone_LStick_Y = 10;

    options.InputOption.Duration = 100;
    options.InputOption.Interval = 400;

    Memory_Copy(options.InputOption.Maping, controllerMapping, 96);

    if (setTestMacro)
        options.InputOption.Maping[0] = 0x0800; //set triangle button to macro

    NtDeviceIoControlFile(driverHandle, NULL, NULL, NULL, &isb, IOCTL_MIJ_SET_CONFIG_OPTIONS, &options, 256, NULL, 0);
    File_Close(driverHandle);
}


VOID SetMacro( UINT8 Count, MOTIONINJOY_MACRO* Macro )
{
    HANDLE driverHandle;
    BYTE b;
    MOTIONINJOY_BUTTON_MACRO macro;
    IO_STATUS_BLOCK isb;

    File_Create(
        &driverHandle,
        L"\\\\.\\MIJFilter",
        SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    Memory_Clear(&macro, sizeof(MOTIONINJOY_BUTTON_MACRO));

    macro.DeviceIndex = 0;
    macro.EIndex = 0;

    b = 0;

    while ((int)b < ((Count > 6) ? 6 : Count))
    {
        macro.MacroIndex = b;
        macro.Duration = Macro->Duration;

        Memory_Copy(macro.Command, Macro->Command, sizeof(macro.Command));

        int sizzer = sizeof(MOTIONINJOY_BUTTON_MACRO);

        // Fill buttons
        macro.ButtonIndex = 0;
        Memory_Copy(macro.Button, Macro->Button_1, sizeof(macro.Button));
        NtDeviceIoControlFile(driverHandle, NULL, NULL, NULL, &isb, IOCTL_MIJ_SET_CONFIG_MACRO, &macro, sizeof(MOTIONINJOY_BUTTON_MACRO), NULL, 0);

        macro.ButtonIndex = 1;
        Memory_Copy(macro.Button, Macro->Button_2, sizeof(macro.Button));
        NtDeviceIoControlFile(driverHandle, NULL, NULL, NULL, &isb, IOCTL_MIJ_SET_CONFIG_MACRO, &macro, sizeof(MOTIONINJOY_BUTTON_MACRO), NULL, 0);
        
        macro.ButtonIndex = 2;
        Memory_Copy(macro.Button, Macro->Button_3, sizeof(macro.Button));
        NtDeviceIoControlFile(driverHandle, NULL, NULL, NULL, &isb, IOCTL_MIJ_SET_CONFIG_MACRO, &macro, sizeof(MOTIONINJOY_BUTTON_MACRO), NULL, 0);
        
        macro.ButtonIndex = 3;
        Memory_Copy(macro.Button, Macro->Button_4, sizeof(macro.Button));
        NtDeviceIoControlFile(driverHandle, NULL, NULL, NULL, &isb, IOCTL_MIJ_SET_CONFIG_MACRO, &macro, sizeof(MOTIONINJOY_BUTTON_MACRO), NULL, 0);
        
        macro.ButtonIndex = 4;
        Memory_Copy(macro.Button, Macro->Button_5, sizeof(macro.Button));
        NtDeviceIoControlFile(driverHandle, NULL, NULL, NULL, &isb, IOCTL_MIJ_SET_CONFIG_MACRO, &macro, sizeof(MOTIONINJOY_BUTTON_MACRO), NULL, 0);
        
        macro.ButtonIndex = 5;
        Memory_Copy(macro.Button, Macro->Button_6, sizeof(macro.Button));
        NtDeviceIoControlFile(driverHandle, NULL, NULL, NULL, &isb, IOCTL_MIJ_SET_CONFIG_MACRO, &macro, sizeof(MOTIONINJOY_BUTTON_MACRO), NULL, 0);

        b++;
        Macro++;
    }

    File_Close(driverHandle);
}
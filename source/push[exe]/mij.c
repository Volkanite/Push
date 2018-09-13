#include <sl.h>
#include "push.h"


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
    0x0308  //E
};

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


VOID SetProfile( WCHAR* GameName )
{
    HANDLE driverHandle;
    IO_STATUS_BLOCK isb;
    MOTIONINJOY_APP_OPTION options;
    wchar_t bigbuff[260];
    void* buttonMapping = NULL;
    wchar_t* settingsFile = NULL;
    BOOLEAN setTestMacro = FALSE;

    if (setTestMacro)
    {
        MOTIONINJOY_MACRO macro;

        Memory_Clear(&macro, sizeof(MOTIONINJOY_MACRO));

        macro.Duration = 800; //800 milli-seconds
        Memory_Copy(macro.Command, SomeMacro, sizeof(macro.Command));

        macro.ButtonIntervals[0][10] = 0x01; //0ms
        macro.ButtonIntervals[1][10] = 0x02; //100ms
        macro.ButtonIntervals[2][10] = 0x04; //200ms
        macro.ButtonIntervals[3][10] = 0x08; //300ms
        macro.ButtonIntervals[4][10] = 0x10; //400ms
        macro.ButtonIntervals[5][10] = 0x20; //500ms

        SetMacro(1, &macro);
    }

    GetControllerMappingFile(GameName, bigbuff);

    buttonMapping = File_Load(bigbuff, NULL);

    GetSettingsFile(GameName, bigbuff);

    settingsFile = File_Load(bigbuff, NULL);

    if (!settingsFile || !buttonMapping)
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
    options.CommonOption.AutoOff_timeout = 0x80 | PushSharedMemory->ControllerTimeout;
    options.CommonOption.Deadzone_LStick_X = 10;
    options.CommonOption.Deadzone_LStick_Y = 10;

    options.InputOption.Duration = 100;
    options.InputOption.Interval = 400;

    Memory_Copy(options.InputOption.Maping, buttonMapping, 96);

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
        int i;

        macro.MacroIndex = b;
        macro.Duration = Macro->Duration;

        Memory_Copy(macro.Command, Macro->Command, sizeof(macro.Command));

        // Fill button intervals

        for (i = 0; i < 6; i++)
        {
            macro.ButtonIndex = i;

            Memory_Copy(macro.Button, Macro->ButtonIntervals[i], sizeof(macro.Button));

            NtDeviceIoControlFile(
                driverHandle,
                NULL, NULL, NULL,
                &isb,
                IOCTL_MIJ_SET_CONFIG_MACRO,
                &macro,
                sizeof(MOTIONINJOY_BUTTON_MACRO),
                NULL,
                0
                );
        }

        b++;
        Macro++;
    }

    File_Close(driverHandle);
}

HANDLE __stdcall SetupDiGetClassDevsW(
    GUID* ClassGuid,
    WCHAR* Enumerator,
    HANDLE hwndParent,
    DWORD Flags
    );

INTBOOL __stdcall SetupDiEnumDeviceInfo(
    HANDLE DeviceInfoSet,
    DWORD MemberIndex,
    SP_DEVINFO_DATA* DeviceInfoData
    );

INTBOOL __stdcall SetupDiGetDeviceRegistryPropertyW(
    HANDLE DeviceInfoSet,
    SP_DEVINFO_DATA* DeviceInfoData,
    DWORD Property,
    DWORD* PropertyRegDataType,
    BYTE* PropertyBuffer,
    DWORD PropertyBufferSize,
    DWORD* RequiredSize
    );

DWORD __stdcall DriverPackageInstallW(
    WCHAR* DriverPackageInfPath,
    DWORD  Flags,
    void* pInstallerInfo,
    INTBOOL  *pNeedReboot
    );

#define DIGCF_PRESENT           0x00000002
#define DIGCF_ALLCLASSES        0x00000004

#define SPDRP_HARDWAREID                  (0x00000001)  // HardwareID (R/W)
#define SPDRP_COMPATIBLEIDS               (0x00000002)  // CompatibleIDs (R/W)

typedef struct _USB_DEVICES
{
    wchar_t* RegistryId;
    wchar_t* FriendlyName;
}USB_DEVICES;

USB_DEVICES KnownDevices[] = {
    { L"USB\\Vid_054c&Pid_0268", L"Sony PlayStation 3 Controller" },
    { L"USB\\Vid_0A12&Pid_0001", L"TRENDnet TBW-106UB Bluetooth Adapter" },
    { L"USB\\Vid_0B05&Pid_17CB", L"ASUS USB-BT400 Bluetooth Adapter" }
    // Add more devices to search for here
};

WCHAR* String_CompareIgnoreCaseN(WCHAR* s1, WCHAR* s2, int n);
void InstallDriver(HANDLE DeviceInformation, SP_DEVINFO_DATA* DeviceData);

void EnumerateDevices()
{
    HANDLE deviceInfo = NULL;

    deviceInfo = SetupDiGetClassDevsW(NULL, L"USB", NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);

    if (deviceInfo != INVALID_HANDLE_VALUE)
    {
        BOOLEAN flag = TRUE;
        unsigned int num = 0;

        while (flag)
        {
            SP_DEVINFO_DATA deviceData;

            Memory_Clear(&deviceData, sizeof(SP_DEVINFO_DATA));

            deviceData.cbSize = sizeof(SP_DEVINFO_DATA);
            deviceData.DevInst = NULL;
            deviceData.Reserved = 0;

            flag = SetupDiEnumDeviceInfo(deviceInfo, num, &deviceData);
            
            if (flag)
            {
                wchar_t propertyBuffer[1000];
                DWORD requiredSize = 0;
                DWORD propertyDataType = 1;

                if (SetupDiGetDeviceRegistryPropertyW(
                    deviceInfo, 
                    &deviceData, 
                    SPDRP_HARDWAREID,
                    &propertyDataType, 
                    &propertyBuffer, 
                    1000 * sizeof(wchar_t), 
                    &requiredSize))
                {
                    BOOLEAN flag2 = FALSE;

                    // Direct Match, definitely a device we recognize
                    for (int i = 0; i < sizeof(KnownDevices) / sizeof(KnownDevices[0]); i++)
                    {
                        if (String_CompareIgnoreCaseN(propertyBuffer, KnownDevices[i].RegistryId, 22))
                        {
                            flag2 = TRUE;
                            Log(L"found %s [%s]", KnownDevices[i].FriendlyName, propertyBuffer);
                            break;
                        }
                    }

                    // Possible devices that could work
                    if (!flag2 
                        && SetupDiGetDeviceRegistryPropertyW(
                            deviceInfo, 
                            &deviceData, 
                            SPDRP_COMPATIBLEIDS, 
                            &propertyDataType,
                            &propertyBuffer, 
                            1000 * sizeof(wchar_t), 
                            &requiredSize))
                    {
                        //Bluetooth Device
                        //BaseClass[0xE0] = Wireless Controller, SubClass[0x01] = Wireless Radio, Protocol[0x01] = Bluetooth Programming Interface)
                        //https://web.archive.org/web/20070626033649/http://www.usb.org/developers/defined_class/#BaseClassE0h
                        if (String_CompareIgnoreCaseN(propertyBuffer, L"usb\\Class_e0&subClass_01&Prot_01", 33))
                        {
                            Log(L"found Bluetooth device %s", propertyBuffer);
                            flag2 = TRUE;
                        }
                        //Controller
                        else if (String_CompareIgnoreCaseN(propertyBuffer, L"usb\\Class_58&subClass_42", 25))
                        {
                            Log(L"found Controller %s", propertyBuffer);
                            flag2 = TRUE;
                        }
                    }
                    if (flag2)
                    {
                        //Callback(&deviceData);
                        InstallDriver(deviceInfo, &deviceData);
                    }
                }
            }

            num += 1;
        }
    }
}


#define BOOL INTBOOL

INTBOOL __stdcall SetupDiSetSelectedDevice(
    HANDLE DeviceInfoSet,
    SP_DEVINFO_DATA* DeviceInfoData
    );
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

BOOL __stdcall SetupDiCallClassInstaller(
    UINT32 InstallFunction,
    HANDLE DeviceInfoSet,
    SP_DEVINFO_DATA* DeviceInfoData
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

DWORD __stdcall GetLastError();
#define SPDIT_COMPATDRIVER       0x00000002


void InstallDriver( HANDLE DeviceInformation, SP_DEVINFO_DATA* DeviceData )
{
    SP_DEVINSTALL_PARAMS_W deviceInstallParams;
    SP_DRVINFO_DATA_V2_W driverData;
    UINT32 reboot = 0;
    
    if (!SetupDiSetSelectedDevice(DeviceInformation, DeviceData))
    {
        Log(L"Set Selected Device fail.");
        return;
    }

    Memory_Clear(&deviceInstallParams, sizeof(SP_DEVINSTALL_PARAMS_W));
    
    if (sizeof(int*) == 8)
    {
        deviceInstallParams.cbSize = 584;
    }
    else
    {
        deviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS_W);
    }

    if (!SetupDiGetDeviceInstallParamsW(DeviceInformation, DeviceData, &deviceInstallParams))
    {
        Log(L"Get Device Install Params fail.");
        return;
    }

    String_Copy(deviceInstallParams.DriverPath, L"C:\\mij\\drivers\\MijXinput.inf");

    deviceInstallParams.Flags |= 65536;

    if (!SetupDiSetDeviceInstallParamsW(DeviceInformation, DeviceData, &deviceInstallParams))
    {
        Log(L"Set Device Install Params fail.");
        return;
    }

    if (!SetupDiBuildDriverInfoList(DeviceInformation, DeviceData, SPDIT_COMPATDRIVER))
    {
        Log(L"Building Driver Info List fail.");
        return;
    }

    if (!SetupDiSelectBestCompatDrv(DeviceInformation, DeviceData))
    {
        Log(L"Select Best Compatible Driver fail.");
        return;
    }
    
    Memory_Clear(&driverData, sizeof(SP_DRVINFO_DATA_V2_W));
    driverData.cbSize = sizeof(SP_DRVINFO_DATA_V2_W);

    if (!SetupDiGetSelectedDriverW(DeviceInformation, DeviceData, &driverData))
    {
        Log(L"Get MotioninJoy Driver fail.");
        return;
    }

    //DiInstallDevice willfully returns 0xE0000235 (ERROR_IN_WOW64) in a WOW64 environment.
    //I don't know if this was a security restraint but there is no reason why this function
    //should not work under WOW64. All we have to do is insert one, literally one jmp patch to
    //skip the WOW64 check and the function succeeds as normal.
    DWORD module = Module_GetHandle(L"newdev.dll");
    DWORD address = Module_GetProcedureAddress(module, "DiInstallDevice");
    address += 0x134;
    BYTE *addr = address;

    if ((*addr) == 0x74) //je
    {
        DWORD oldPageProtection = 0;

        //We firstly have to remove page protection of course.
        VirtualProtect(addr, 1, PAGE_EXECUTE_READWRITE, &oldPageProtection);
        
        //patch to jne
        *addr = 0x75;

        //Lastly, make it look like we were never even there by restoring protection
        VirtualProtect(addr, 1, oldPageProtection, &oldPageProtection);
    }

    if (!DiInstallDevice(NULL, DeviceInformation, DeviceData, &driverData, 0, &reboot))
    {
        Log(L"Install MotioninJoy Driver fail.");
    }
}
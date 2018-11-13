#include <sl.h>
#include "push.h"


USB_DEVICES KnownDevices[] = {
    { L"USB\\Vid_054c&Pid_0268", L"Sony PlayStation 3 Controller" },
    { L"USB\\Vid_0A12&Pid_0001", L"TRENDnet TBW-106UB Bluetooth Adapter" },
    { L"USB\\Vid_0B05&Pid_17CB", L"ASUS USB-BT400 Bluetooth Adapter" }
    // Add more devices to search for here
};


void GetControllerMappingFile( wchar_t* GameName, wchar_t* Buffer )
{
    wchar_t *dot;
    wchar_t batchFile[260];

    String_Copy(batchFile, L"mij\\");
    String_Concatenate(batchFile, GameName);

    dot = String_FindLastChar(batchFile, '.');

    if (dot)
        String_Copy(dot, L".map");
    else
        String_Concatenate(batchFile, L".map");

    String_Copy(Buffer, batchFile);
}


void GetSettingsFile( wchar_t* GameName, wchar_t* Buffer )
{
    wchar_t *dot;
    wchar_t batchFile[260];

    String_Copy(batchFile, L"mij\\");
    String_Concatenate(batchFile, GameName);

    dot = String_FindLastChar(batchFile, '.');

    if (dot)
        String_Copy(dot, L".mij");
    else
        String_Concatenate(batchFile, L".mij");

    String_Copy(Buffer, batchFile);
}


void SetMacro(UINT8 Count, MOTIONINJOY_MACRO* Macro);
void InstallDriver(HANDLE DeviceInformation, SP_DEVINFO_DATA* DeviceData);
void GetDeviceDetail(HANDLE DeviceInformation, SP_DEVINFO_DATA* DeviceData);


WORD SomeMacro[6] = {
    0x0214, //D-Pad Up
    0x0216, //D-Pad Down
    0x0215, //D-Pad Right
    0x0217, //D-Pad Left
    0x0215, //D-Pad Right
    0x0308  //E
};


VOID Mij_SetButton( MOTIONINJOY_BUTTON_MAP* ButtonMapping )
{
    HANDLE driverHandle;
    IO_STATUS_BLOCK isb;
    MOTIONINJOY_APP_OPTION options;

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

    options.CommonOption.mode = DirectInput;
    options.CommonOption.LED = 129;
    options.CommonOption.AutoOff_timeout = 0x80 | /*PushSharedMemory->ControllerTimeout*/10; //minutes
    options.CommonOption.Deadzone_LStick_X = 10;
    options.CommonOption.Deadzone_LStick_Y = 10;

    options.InputOption.Duration = 100;
    options.InputOption.Interval = 400;

    Memory_Copy(&options.InputOption.Maping, ButtonMapping, sizeof(MOTIONINJOY_BUTTON_MAP));

    NtDeviceIoControlFile(driverHandle, NULL, NULL, NULL, &isb, IOCTL_MIJ_SET_CONFIG_OPTIONS, &options, 256, NULL, 0);
    File_Close(driverHandle);
}


VOID Mij_SetProfile( WCHAR* GameName )
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

    Memory_Copy(&options.InputOption.Maping, buttonMapping, sizeof(MOTIONINJOY_BUTTON_MAP));

    if (setTestMacro)
        options.InputOption.Maping.Triangle = 0x0800; //set triangle button to macro

    NtDeviceIoControlFile(driverHandle, NULL, NULL, NULL, &isb, IOCTL_MIJ_SET_CONFIG_OPTIONS, &options, 256, NULL, 0);
    File_Close(driverHandle);
}


void SetMacro( unsigned __int8 Count, MOTIONINJOY_MACRO* Macro )
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


VOID Mij_EnumerateDevices()
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
                        //BaseClass[0xE0] = Wireless Controller, 
                        //SubClass[0x01] = Wireless Radio, 
                        //Protocol[0x01] = Bluetooth Programming Interface
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
                        GetDeviceDetail(deviceInfo, &deviceData);
                    }
                }
            }

            num += 1;
        }
    }
}


void GetDeviceDetail( HANDLE DeviceInformation, SP_DEVINFO_DATA* DeviceData )
{
    wchar_t propertyBuffer[1000];
    DWORD propertyDataType = 1;
    DWORD requiredSize = 0;

    SetupDiGetDeviceRegistryPropertyW(
        DeviceInformation,
        DeviceData,
        SPDRP_DEVICEDESC,
        &propertyDataType,
        &propertyBuffer,
        1000 * sizeof(wchar_t),
        &requiredSize
        );

    if (String_Compare(propertyBuffer, L"MotioninJoy Virtual Xinput device for Windows") == 0)
    {
        Log(L"Driver already installed for this device");
    }
    else
    {
        Log(L"Installing driver...");
        InstallDriver(DeviceInformation, DeviceData);
    }
}


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
    HANDLE module = Module_GetHandle(L"newdev.dll");
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

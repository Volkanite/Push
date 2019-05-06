#include <Windows.h>
#include <hexus.h>
#include <dinput.h>


DETOUR_PROPERTIES dinptdt;
LPDIENUMDEVICESCALLBACKA D3D9Hook_enumcallbck;
BOOLEAN SpoofControllerType;

/* Direct Input */

typedef HRESULT(__stdcall *TYPE_DirectInput8Create)(
    HINSTANCE hinst,
    DWORD dwVersion,
    REFIID riidltf,
    LPVOID* ppvOut,
    LPUNKNOWN punkOuter
    );

/* IDirectInput8 */

typedef HRESULT(__stdcall *TYPE_IDirectInput8_CreateDevice)(
    IDirectInput8A* Device,
    REFGUID rguid,
    LPDIRECTINPUTDEVICE8A* lplpDirectInputDevice,
    LPUNKNOWN pUnkOuter
    );

typedef HRESULT(__stdcall *TYPE_IDirectInput8_EnumDevices)(
    IDirectInput8A* Device,
    DWORD dwDevType,
    LPDIENUMDEVICESCALLBACKA lpCallback,
    LPVOID pvRef,
    DWORD dwFlags
    );

typedef HRESULT(__stdcall *TYPE_IDirectInputDevice8_GetCapabilities)(
    IDirectInputDevice8A *device,
    LPDIDEVCAPS lpDIDevCaps
    );

typedef HRESULT(__stdcall *TYPE_IDirectInputDevice8_GetDeviceInfo)(
    IDirectInputDevice8A *device,
    LPDIDEVICEINSTANCEA pdidi
    );


TYPE_DirectInput8Create                     D3D9Hook_DirectInput8Create;
TYPE_IDirectInput8_CreateDevice             D3D9Hook_IDirectInput8_CreateDevice;
TYPE_IDirectInput8_EnumDevices              D3D9Hook_IDirectInput8_EnumDevices;
TYPE_IDirectInputDevice8_GetCapabilities    D3D9Hook_IDirectInputDevice8_GetCapabilities;
TYPE_IDirectInputDevice8_GetDeviceInfo      D3D9Hook_IDirectInputDevice8_GetDeviceInfo;
DETOUR_PROPERTIES Detour_DirectInput_CreateDevice;
DETOUR_PROPERTIES Detour_DirectInput_EnumDevices;
DETOUR_PROPERTIES Detour_DirectInputDevice_GetCapabilities;
DETOUR_PROPERTIES Detour_DirectInputDevice_GetDeviceInfo;

VOID Log(const wchar_t* Format, ...);


BOOL __stdcall enumCallbackDetour(const DIDEVICEINSTANCEA* instance, VOID* context)
{
    BOOL result;

    unsigned int type = GET_DIDEVICE_TYPE(instance->dwDevType);
    unsigned int subtype = GET_DIDEVICE_SUBTYPE(instance->dwDevType);

    if (type == DI8DEVTYPE_1STPERSON && subtype == DI8DEVTYPE1STPERSON_SIXDOF)
    {
        if (SpoofControllerType)
            ((DIDEVICEINSTANCEA*)instance)->dwDevType = 0x10215; //DI8DEVTYPE_GAMEPAD | DI8DEVTYPEGAMEPAD_STANDARD
    }

    result = D3D9Hook_enumcallbck(instance, context);

    return result;
}


HRESULT __stdcall IDirectInput8_EnumDevices_Detour(
    IDirectInput8A* Device,
    DWORD dwDevType,
    LPDIENUMDEVICESCALLBACKA lpCallback,
    LPVOID pvRef,
    DWORD dwFlags
    )
{
    HRESULT result;

    D3D9Hook_enumcallbck = (LPDIENUMDEVICESCALLBACKA)lpCallback;

    result = D3D9Hook_IDirectInput8_EnumDevices(Device, dwDevType, enumCallbackDetour, pvRef, dwFlags);

    return result;
}
HRESULT __stdcall  DirectInput8Create_Detour(
    HINSTANCE hinst,
    DWORD dwVersion,
    REFIID riidltf,
    LPVOID* ppvOut,
    LPUNKNOWN punkOuter
    )
{
    HRESULT result;

    result = D3D9Hook_DirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);

    if (result != S_OK)
    {
        return result;
    }

    static bool ronce = FALSE;

    if (!ronce)
    {
        LPDIRECTINPUT8A di;
        DWORD dwFuncTable = (DWORD)*((DWORD*)*ppvOut);

        di = (LPDIRECTINPUT8A)(DWORD)*((DWORD*)*ppvOut);

        //OvLog(L"di 0x%x", di);
        //OvLog(L"dwFuncTable 0x%x!", dwFuncTable);

        VOID **virtualMethodTable;

        virtualMethodTable = (VOID**)/*di;*/dwFuncTable;
        virtualMethodTable = (VOID**)virtualMethodTable[0];

        ronce = TRUE;

        //DWORD oldCreateDevice = *((DWORD*)(dwFuncTable + 0x0C)); //4 * 3 (virtualMethodTable[3]) = 12 = 0x0C
        DWORD oldEnumDevice = *((DWORD*)(dwFuncTable + 0x10)); //4 * 4 (virtualMethodTable[4]) = 16 = 0x10

        DetourCreate(/*virtualMethodTable[4]*/(VOID*)oldEnumDevice, IDirectInput8_EnumDevices_Detour, &Detour_DirectInput_EnumDevices);
        D3D9Hook_IDirectInput8_EnumDevices = (TYPE_IDirectInput8_EnumDevices)Detour_DirectInput_EnumDevices.Trampoline;
    }

    return result;
}


void HookDirectInput()
{
    DWORD base = NULL;
    DWORD func = NULL;
    base = (DWORD)GetModuleHandleW(L"dinput8.dll");

    if (!base)
    {
        return;
    }

    func = (DWORD)GetProcAddress((HMODULE)base, "DirectInput8Create");

    if (!func)
    {
        Log(L"not DirectInput8Create!");
        return;
    }

    Log(L"got DirectInput8Create...");

    DetourCreate((VOID*)func, DirectInput8Create_Detour, &dinptdt);
    D3D9Hook_DirectInput8Create = (TYPE_DirectInput8Create)dinptdt.Trampoline;
}
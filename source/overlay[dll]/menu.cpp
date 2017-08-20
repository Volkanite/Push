#include <windows.h>
#include <OvRender.h>
#include <sloverlay.h>
#include <overlay.h>

#include "kbhook.h"
#include "osd.h"


SlOverlayMenu* Menu;

MenuVars MenuOsd[30];
MenuVars MenuGpu[10];
MenuVars Diagnostics[5];
MenuVars D3DTweaks[10];
MenuVars Process[5];

WCHAR* GroupOpt[] = {L">", L"<"};
WCHAR* ItemOpt[] = {L"Off", L"On"};
WCHAR* PressOpt[] = { L">>", L">>" };
WCHAR* GpuSpeedEngineOpt[] = { L"", L"" };
WCHAR* GpuSpeedMemoryOpt[] = { L"", L"" };
WCHAR* GpuVoltageOpt[] = { L"", L"" };
WCHAR* GpuFanDutyCycleOpt[] = { L"", L"" };
WCHAR* FrameLimitOpt[] = { L"", L"" };

WCHAR GpuSpeedEngine[20];
WCHAR GpuSpeedMemory[20];
WCHAR GpuVoltage[20];
WCHAR GpuFanDutyCycle[20];
WCHAR FrameLimitText[20];

BOOLEAN MnuInitialized;
HANDLE MenuProcessHeap;

extern OV_WINDOW_MODE D3D9Hook_WindowMode;
extern BOOLEAN D3D9Hook_ForceReset;
extern BOOLEAN DisableAutoOverclock;
extern UINT8 FrameLimit;
extern OSD_VARS Variables;


#define ID_RESET            OSD_LAST_ITEM+1
#define ID_FORCEMAX         OSD_LAST_ITEM+2
#define ID_ECLOCK           OSD_LAST_ITEM+3
#define ID_MCLOCK           OSD_LAST_ITEM+4
#define ID_VOLTAGE          OSD_LAST_ITEM+5
#define ID_FILELOGGING      OSD_LAST_ITEM+6
#define ID_FILEAUTOLOG      OSD_LAST_ITEM+7
#define ID_WINDOWED         OSD_LAST_ITEM+8
#define ID_KEEPACTIVE       OSD_LAST_ITEM+9
#define ID_FRAMELIMITER     OSD_LAST_ITEM+10
#define ID_FRAMELIMIT       OSD_LAST_ITEM+11
#define ID_VSYNC            OSD_LAST_ITEM+12
#define ID_TERMINATE        OSD_LAST_ITEM+13
#define ID_KEEP_FPS         OSD_LAST_ITEM+14
#define ID_OC               OSD_LAST_ITEM+15
#define ID_FAN              OSD_LAST_ITEM+16
#define ID_API              OSD_LAST_ITEM+17
#define ID_FRAMETIME        OSD_LAST_ITEM+18


//Add menu items to menu
#include <stdio.h>
#include <wchar.h>
VOID ChangeVsync(BOOLEAN Setting);


VOID UpdateGpuInformation()
{
    swprintf(GpuSpeedEngine, 20, L"%i MHz", PushSharedMemory->HarwareInformation.DisplayDevice.EngineClockMax);
    GpuSpeedEngineOpt[0] = GpuSpeedEngine;
    GpuSpeedEngineOpt[1] = GpuSpeedEngine;

    swprintf(GpuSpeedMemory, 20, L"%i MHz", PushSharedMemory->HarwareInformation.DisplayDevice.MemoryClockMax);
    GpuSpeedMemoryOpt[0] = GpuSpeedMemory;
    GpuSpeedMemoryOpt[1] = GpuSpeedMemory;

    swprintf(GpuVoltage, 20, L"%i mV", PushSharedMemory->HarwareInformation.DisplayDevice.VoltageMax);
    GpuVoltageOpt[0] = GpuVoltage;
    GpuVoltageOpt[1] = GpuVoltage;

    swprintf(GpuFanDutyCycle, 20, L"%i %%", PushSharedMemory->HarwareInformation.DisplayDevice.FanDutyCycle);
    GpuFanDutyCycleOpt[0] = GpuFanDutyCycle;
    GpuFanDutyCycleOpt[1] = GpuFanDutyCycle;
}


VOID UpdateFrameLimitText()
{
    swprintf(FrameLimitText, 20, L"%i", FrameLimit);

    FrameLimitOpt[0] = FrameLimitText;
    FrameLimitOpt[1] = FrameLimitText;
}


VOID AddItems()
{
    Menu->AddGroup(L"OSD", GroupOpt, &MenuOsd[0]);

    if (MenuOsd[0].Var)
    {
        UINT8 i = 1;

        Menu->AddItem(L"GPU Core utilization",    ItemOpt, &MenuOsd[i++], OSD_GPU_LOAD);
        Menu->AddItem(L"GPU Core temperature",    ItemOpt, &MenuOsd[i++], OSD_GPU_TEMP);
        Menu->AddItem(L"GPU Engine Clock",        ItemOpt, &MenuOsd[i++], OSD_GPU_E_CLK);
        Menu->AddItem(L"GPU Memory Clock",        ItemOpt, &MenuOsd[i++], OSD_GPU_M_CLK);
        Menu->AddItem(L"GPU VRAM usage",          ItemOpt, &MenuOsd[i++], OSD_GPU_VRAM);
        Menu->AddItem(L"GPU Fan Speed",           ItemOpt, &MenuOsd[i++], OSD_GPU_FAN_RPM);
        Menu->AddItem(L"GPU Fan Duty Cycle",      ItemOpt, &MenuOsd[i++], OSD_GPU_FAN_DC);
        Menu->AddItem(L"CPU Speed",               ItemOpt, &MenuOsd[i++], OSD_CPU_SPEED);
        Menu->AddItem(L"CPU utilization",         ItemOpt, &MenuOsd[i++], OSD_CPU_LOAD);
        Menu->AddItem(L"CPU temperature",         ItemOpt, &MenuOsd[i++], OSD_CPU_TEMP);
        Menu->AddItem(L"RAM usage",               ItemOpt, &MenuOsd[i++], OSD_RAM);
        //Menu->AddItem(L"Max core usage",          ItemOpt, &MenuOsd[i++], OSD_MCU);
        //Menu->AddItem(L"Max thread usage",        ItemOpt, &MenuOsd[i++], OSD_MTU);
        Menu->AddItem(L"Disk read-write rate",    ItemOpt, &MenuOsd[i++], OSD_DISK_RWRATE);
        Menu->AddItem(L"Disk response time",      ItemOpt, &MenuOsd[i++], OSD_DISK_RESPONSE);
        Menu->AddItem(L"Frame Buffer count",      ItemOpt, &MenuOsd[i++], OSD_BUFFERS);
        Menu->AddItem(L"Resolution",              ItemOpt, &MenuOsd[i++], OSD_RESOLUTION);
        Menu->AddItem(L"Resfresh Rate",           ItemOpt, &MenuOsd[i++], OSD_REFRESH_RATE);
        Menu->AddItem(L"Frame Time",              ItemOpt, &MenuOsd[i++], ID_FRAMETIME);
        Menu->AddItem(L"API",                     ItemOpt, &MenuOsd[i++], ID_API);
        Menu->AddItem(L"Time",                    ItemOpt, &MenuOsd[i++], OSD_TIME);
        Menu->AddItem(L"FPS",                     ItemOpt, &MenuOsd[i++], ID_KEEP_FPS);
        Menu->AddItem(L"Reset",                   PressOpt, &MenuOsd[i++], ID_RESET);
    }

    Menu->AddGroup(L"GPU", GroupOpt, &MenuGpu[0]);

    if (MenuGpu[0].Var)
    {
        Menu->AddItem(L"Force Max Clocks", ItemOpt, &MenuGpu[1], ID_FORCEMAX);
        Menu->AddItem(L"Auto-Overclock", ItemOpt, &MenuGpu[2], ID_OC);
        Menu->AddItem(L"Engine Clock", GpuSpeedEngineOpt, &MenuGpu[3], ID_ECLOCK);
        Menu->AddItem(L"Memory Clock", GpuSpeedMemoryOpt, &MenuGpu[4], ID_MCLOCK);
        Menu->AddItem(L"Voltage", GpuVoltageOpt, &MenuGpu[5], ID_VOLTAGE);
        Menu->AddItem(L"Fan Duty Cycle", GpuFanDutyCycleOpt, &MenuGpu[6], ID_FAN);

        //Init gpu information
        UpdateGpuInformation();
    }

    Menu->AddGroup(L"Diagnostics", GroupOpt, &Diagnostics[0]);

    if (Diagnostics[0].Var)
    {
        Menu->AddItem(L"File Logging", ItemOpt, &Diagnostics[1], ID_FILELOGGING);
        Menu->AddItem(L"Auto-log", ItemOpt, &Diagnostics[2], ID_FILEAUTOLOG);
    }

    Menu->AddGroup(L"Direct3D", GroupOpt, &D3DTweaks[0]);

    if (D3DTweaks[0].Var)
    {
        Menu->AddItem(L"Windowed", ItemOpt, &D3DTweaks[1], ID_WINDOWED);
        Menu->AddItem(L"Keep active", ItemOpt, &D3DTweaks[2], ID_KEEPACTIVE);
        Menu->AddItem(L"Frame Limiter", ItemOpt, &D3DTweaks[3], ID_FRAMELIMITER);
        Menu->AddItem(L"Frame Limit", FrameLimitOpt, &D3DTweaks[4], ID_FRAMELIMIT);
        Menu->AddItem(L"Vsync", ItemOpt, &D3DTweaks[5], ID_VSYNC);

        UpdateFrameLimitText();
    }

    Menu->AddGroup(L"Process", GroupOpt, &Process[0]);

    if (Process[0].Var)
    {
        Menu->AddItem(L"Terminate", ItemOpt, &Process[1], ID_TERMINATE);
    }
}


typedef enum _OVERCLOCK_UNIT
{
    OC_ENGINE,
    OC_MEMORY,
    OC_VOLTAGE

}OVERCLOCK_UNIT;


VOID Overclock( OVERCLOCK_UNIT Unit )
{
    switch (Unit)
    {
    case OC_ENGINE:
        PushSharedMemory->HarwareInformation.DisplayDevice.EngineClockMax++;
        PushSharedMemory->OSDFlags |= OSD_GPU_E_CLK;
        break;
    case OC_MEMORY:
        PushSharedMemory->HarwareInformation.DisplayDevice.MemoryClockMax++;
        PushSharedMemory->OSDFlags |= OSD_GPU_M_CLK;
        break;
    case OC_VOLTAGE:
        PushSharedMemory->HarwareInformation.DisplayDevice.VoltageMax++;
        PushSharedMemory->OSDFlags |= OSD_GPU_VOLTAGE;
    default:
        break;
    }

    CallPipe(L"UpdateClocks", NULL);
    UpdateGpuInformation();
}


VOID ProcessOptions( MenuItems* Item )
{
    switch (*Item->Id)
    {
    case ID_KEEP_FPS:
        if (*Item->Var > 0)
            PushSharedMemory->KeepFps = TRUE;
        else
            PushSharedMemory->KeepFps = FALSE;
        break;

    case ID_RESET:
        PushSharedMemory->Overloads = 0;
        PushSharedMemory->OSDFlags = 0;
        PushSharedMemory->OSDFlags |= OSD_FPS;

        for (int i = 1; i < 20; i++)
        {
            MenuOsd[i].Var = 0;
        }
        break;

    case ID_FORCEMAX:
        CallPipe(L"ForceMaxClocks", NULL);

        PushSharedMemory->OSDFlags |= OSD_GPU_E_CLK;
        PushSharedMemory->OSDFlags |= OSD_GPU_M_CLK;
        PushSharedMemory->Overloads &= ~OSD_GPU_E_CLK;
        PushSharedMemory->Overloads &= ~OSD_GPU_M_CLK;
        break;

    case ID_ECLOCK:
        {
            DisableAutoOverclock = TRUE;

            switch (*Item->Var)
            {
            case 0:
            {
                PushSharedMemory->HarwareInformation.DisplayDevice.EngineClockMax--;

                UpdateGpuInformation();
                CallPipe(L"UpdateClocks", NULL);
            }
            break;

            case 1:
            {
                Overclock(OC_ENGINE);
            }
            break;
            }
        }
        break;

    case ID_MCLOCK:
        {
            if (*Item->Var > 0)
            {
                Overclock(OC_MEMORY);
            }
        }
        break;

    case ID_VOLTAGE:
    {
        switch (*Item->Var)
        {
        case 0:
        {
            PushSharedMemory->HarwareInformation.DisplayDevice.Voltage--;

            UpdateGpuInformation();
            CallPipe(L"UpdateClocks", NULL);
        }
        break;

        case 1:
        {
            Overclock(OC_VOLTAGE);
        }
        break;
        }
    }
    break;

    case ID_FAN:
    {
        PushSharedMemory->OSDFlags |= OSD_GPU_FAN_DC;

        if (*Item->Var > 0)
            PushSharedMemory->HarwareInformation.DisplayDevice.FanDutyCycle++;
        else
            PushSharedMemory->HarwareInformation.DisplayDevice.FanDutyCycle--;

        UpdateGpuInformation();
        CallPipe(L"UpdateFanSpeed", NULL);
    }
    break;

    case ID_OC:
        if (*Item->Var > 0)
            DisableAutoOverclock = FALSE;
        else
            DisableAutoOverclock = TRUE;
        break;

    case ID_FILELOGGING:
        if (*Item->Var > 0)
            PushSharedMemory->LogFileIo = TRUE;
        else
            PushSharedMemory->LogFileIo = FALSE;
        break;

    case ID_FILEAUTOLOG:
        if (*Item->Var > 0)
            PushSharedMemory->AutoLogFileIo = TRUE;
        else
            PushSharedMemory->AutoLogFileIo = FALSE;
        break;

    case ID_WINDOWED:
        if (*Item->Var > 0)
        {
            D3D9Hook_WindowMode = WINDOW_WINDOWED;
            D3D9Hook_ForceReset = TRUE;
        }
        else
        {
            D3D9Hook_WindowMode = WINDOW_FULLSCREEN;
            D3D9Hook_ForceReset = TRUE;
        }
        break;

    case ID_KEEPACTIVE:
        if (*Item->Var > 0)
        {
            Keyboard_Hook(KEYBOARD_HOOK_SUBCLASS);
        }
        break;

    case ID_FRAMELIMITER:
        if (*Item->Var > 0)
        {
            PushSharedMemory->FrameLimit = TRUE;
        }
        else
        {
            PushSharedMemory->FrameLimit = FALSE;
        }
        break;

    case ID_FRAMELIMIT:
        {
            if (*Item->Var == 0)
            {
                FrameLimit--;
            }
            else if (*Item->Var == 1)
            {
                FrameLimit++;
            }
                 
            UpdateFrameLimitText();
        }
        break;

    case ID_VSYNC:
        if (*Item->Var > 0)
        {
            ChangeVsync(TRUE);
        }
        else
        {
            ChangeVsync(FALSE);
        }
        break;

    case ID_FRAMETIME:
        if (*Item->Var > 0)
            Variables.FrameTime = TRUE;
        else
            Variables.FrameTime = FALSE;
        break;

    case ID_API:
        if (*Item->Var > 0)
            Variables.GraphicsApi = TRUE;
        else
            Variables.GraphicsApi = FALSE;
        break;

    case ID_TERMINATE:
        if (*Item->Var > 0)
        {
            TerminateProcess(GetCurrentProcess(), 0);
        }
        break;

    default:
        if (*Item->Var > 0)
            PushSharedMemory->OSDFlags |= *Item->Id;
        else
            PushSharedMemory->OSDFlags &= ~*Item->Id;
        break;
    }
}


VOID MnuRender( OvOverlay* Overlay )
{
    if (!MnuInitialized)
    {
        Menu = new SlOverlayMenu(300);
        MnuInitialized = TRUE;
    }

    if( Menu->mSet.MaxItems == 0 )
        AddItems();

    //Call drawing and navigation functions
    Menu->Render(100, 200, Overlay);
}


#define GROUP 1
#define ITEM  2
#define HEIGHT 16


DWORD LightBlue = 0xFF4DD0EB;
DWORD Green     = 0xFF33FF00;
DWORD White     = 0xFFE6E6E6;
DWORD Blue      = 0xFF00A4C5;

SlOverlayMenu*  OvmMenu;


VOID MenuKeyboardHook( WPARAM Key )
{
    if (!OvmMenu)
        return;

    if (Key == VK_INSERT || Key == VK_UP || Key == VK_DOWN || Key == VK_LEFT || Key == VK_RIGHT)
    {
        switch (Key)
        {
        case VK_INSERT:

            OvmMenu->mSet.Show = !OvmMenu->mSet.Show;
            
            break;

        case VK_UP:
        {
            if (!OvmMenu->mSet.Show)
                break;

            OvmMenu->mSet.SeletedIndex--;

            if (OvmMenu->mSet.SeletedIndex < 0)
                OvmMenu->mSet.SeletedIndex = OvmMenu->mSet.MaxItems - 1;

        } break;

        case VK_DOWN:
        {
            if (!OvmMenu->mSet.Show)
                break;

            OvmMenu->mSet.SeletedIndex++;

            if (OvmMenu->mSet.SeletedIndex == OvmMenu->mSet.MaxItems)
                OvmMenu->mSet.SeletedIndex = 0;

        } break;

        case VK_LEFT:
            {
                if (!OvmMenu->mSet.Show)
                    break;

                if (OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Var && *OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Var > 0)
                {
                    *OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Var += -1;

                    if (OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Type == GROUP)
                        OvmMenu->mSet.MaxItems = 0;
                }

                if (OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Type == ITEM)
                    ProcessOptions(&OvmMenu->Items[OvmMenu->mSet.SeletedIndex]);
            } 
            break;

        case VK_RIGHT:
            {
                if (!OvmMenu->mSet.Show)
                    break;

                if (OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Var
                    && *OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Var < (OvmMenu->Items[OvmMenu->mSet.SeletedIndex].MaxValue - 1))
                {
                    *OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Var += 1;

                    if (OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Type == GROUP)
                        OvmMenu->mSet.MaxItems = 0;
                }
                
                if (OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Type == ITEM)
                    ProcessOptions(&OvmMenu->Items[OvmMenu->mSet.SeletedIndex]);
            } 
            break;
        }
    }
}


SlOverlayMenu::SlOverlayMenu( int OptionsX )
{
    OpX = OptionsX;
    
    mSet.Show = FALSE;
    mSet.MaxItems = 0;
    mSet.SeletedIndex = 0;
    
    OvmMenu = this;
    MenuProcessHeap = GetProcessHeap();
}


void SlOverlayMenu::AddItemToMenu(WCHAR* Title, WCHAR** Options, MenuVars* Variables, int MaxValue, int Type)
{
    Items[mSet.MaxItems].Title = Title;
    Items[mSet.MaxItems].Options= Options;
    Items[mSet.MaxItems].Var = &Variables->Var;
    Items[mSet.MaxItems].MaxValue = MaxValue;
    Items[mSet.MaxItems].Type = Type;
    Items[mSet.MaxItems].Id = &Variables->Id;

    mSet.MaxItems++;
}


VOID 
SlOverlayMenu::Render( int X, int Y, OvOverlay* Overlay )
{
    DWORD ColorOne, ColorTwo;
    int ValueOne, ValueTwo;

    if (!mSet.Show)
        return;

    for (int i = 0; i < mSet.MaxItems; i++)
    {
        ValueOne = (Items[i].Var) ? (*Items[i].Var) : 0;
        ValueTwo = (Items[i].Var) ? (*Items[i].Var) : 0;

        if (i == mSet.SeletedIndex)
        {
            ColorOne = LightBlue;
            ColorTwo = (ValueTwo) ? Green : White;
        }
        else if (Items[i].Type == GROUP)
        {
            ColorOne = Blue;
            ColorTwo = Blue;
        }
        else
        {
            ColorOne = (ValueOne) ? White : White;
            ColorTwo = (ValueTwo) ? Green : White;
        }

        if (Items[i].Type == GROUP)
        {
            Overlay->DrawText(Items[i].Title, X, Y, ColorOne);

            if (Items[i].Options)
                Overlay->DrawText(Items[i].Options[ValueTwo], 200, Y, ColorTwo);
        }
            

        if (Items[i].Type == ITEM)
        {
            Overlay->DrawText(Items[i].Title, X + 20, Y, ColorOne);

            if (Items[i].Options)
                Overlay->DrawText(Items[i].Options[ValueTwo], OpX, Y, ColorTwo);
        }

        Y += HEIGHT;
    }
}

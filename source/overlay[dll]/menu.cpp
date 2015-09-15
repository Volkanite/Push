#include <windows.h>
#include <OvRender.h>
#include <sloverlay.h>
#include <overlay.h>


SlOverlayMenu* Menu;

MenuVars MenuOsd[20];
MenuVars MenuGpu[10];
MenuVars Diagnostics[5];
MenuVars D3DTweaks[5];

WCHAR* GroupOpt[] = {L"", L""};
WCHAR* ItemOpt[] = {L"Off", L"On"};
WCHAR* PressOpt[] = { L">>", L">>" };
WCHAR* GpuSpeedEngineOpt[] = { L"XXX MHz", L"XXX MHz" };
WCHAR* GpuSpeedMemoryOpt[] = { L"XXX MHz", L"XXX MHz" };
WCHAR* GpuVoltageOpt[] = { L"XXX mV", L"XXX mV" };
WCHAR GpuSpeedEngine[20];
WCHAR GpuSpeedMemory[20];
WCHAR GpuVoltage[20];

BOOLEAN MnuInitialized;
HANDLE MenuProcessHeap;
extern BOOLEAN D3D9Hook_WindowMode;
extern BOOLEAN D3D9Hook_ForceReset;

#define FUNC_RESET          0x00010000
#define FUNC_FORCEMAX       0x00020000
#define FUNC_ECLOCK         0x00040000
#define FUNC_MCLOCK         0x00080000
#define FUNC_VOLTAGE        0x00100000
#define FUNC_FILELOGGING    0x00200000
#define FUNC_FILEAUTOLOG    0x00400000
#define FUNC_WINDOWED       0x00800000


//Add menu items to menu
#include <stdio.h>
#include <wchar.h>


VOID UpdateGpuInformation()
{
    swprintf(GpuSpeedEngine, 20, L"%i MHz", PushSharedMemory->HarwareInformation.DisplayDevice.EngineClock);
    GpuSpeedEngineOpt[0] = GpuSpeedEngine;
    GpuSpeedEngineOpt[1] = GpuSpeedEngine;

    swprintf(GpuSpeedMemory, 20, L"%i MHz", PushSharedMemory->HarwareInformation.DisplayDevice.MemoryClock);
    GpuSpeedMemoryOpt[0] = GpuSpeedMemory;
    GpuSpeedMemoryOpt[1] = GpuSpeedMemory;

    swprintf(GpuVoltage, 20, L"%i mV", PushSharedMemory->HarwareInformation.DisplayDevice.Voltage);
    GpuVoltageOpt[0] = GpuVoltage;
    GpuVoltageOpt[1] = GpuVoltage;
}


VOID AddItems()
{
    Menu->AddGroup(L"OSD >", GroupOpt, &MenuOsd[0]);

    if (MenuOsd[0].Var)
    {
        UINT8 i = 1;

        Menu->AddItem(L"GPU Core utilization",    ItemOpt, &MenuOsd[i++], OSD_GPU_LOAD);
        Menu->AddItem(L"GPU Core temperature",    ItemOpt, &MenuOsd[i++], OSD_GPU_TEMP);
        Menu->AddItem(L"GPU Engine Clock",        ItemOpt, &MenuOsd[i++], OSD_GPU_E_CLK);
        Menu->AddItem(L"GPU Memory Clock",        ItemOpt, &MenuOsd[i++], OSD_GPU_M_CLK);
        Menu->AddItem(L"GPU VRAM usage",          ItemOpt, &MenuOsd[i++], OSD_GPU_VRAM);
        Menu->AddItem(L"GPU Fan Speed",           ItemOpt, &MenuOsd[i++], OSD_GPU_FAN);
        Menu->AddItem(L"CPU utilization",         ItemOpt, &MenuOsd[i++], OSD_CPU_LOAD);
        Menu->AddItem(L"CPU temperature",         ItemOpt, &MenuOsd[i++], OSD_CPU_TEMP);
        Menu->AddItem(L"RAM usage",               ItemOpt, &MenuOsd[i++], OSD_RAM);
        Menu->AddItem(L"Max core usage",          ItemOpt, &MenuOsd[i++], OSD_MCU);
        Menu->AddItem(L"Max thread usage",        ItemOpt, &MenuOsd[i++], OSD_MTU);
        Menu->AddItem(L"Disk read-write rate",    ItemOpt, &MenuOsd[i++], OSD_DISK_RWRATE);
        Menu->AddItem(L"Disk response time",      ItemOpt, &MenuOsd[i++], OSD_DISK_RESPONSE);
        Menu->AddItem(L"Frame Buffer count",      ItemOpt, &MenuOsd[i++], OSD_BUFFERS);
        Menu->AddItem(L"Show Time",               ItemOpt, &MenuOsd[i++], OSD_TIME);

        Menu->AddItem(L"Reset Overlay", PressOpt, &MenuOsd[i++], FUNC_RESET);
    }

    Menu->AddGroup(L"GPU >", GroupOpt, &MenuGpu[0]);

    if (MenuGpu[0].Var)
    {
        Menu->AddItem(L"Force Max Clocks", ItemOpt, &MenuGpu[1], FUNC_FORCEMAX);
        Menu->AddItem(L"Engine Clock", GpuSpeedEngineOpt, &MenuGpu[2], FUNC_ECLOCK);
        Menu->AddItem(L"Memory Clock", GpuSpeedMemoryOpt, &MenuGpu[3], FUNC_MCLOCK);
        Menu->AddItem(L"Voltage", GpuVoltageOpt, &MenuGpu[4], FUNC_VOLTAGE);

        //Init gpu information
        UpdateGpuInformation();
    }

    Menu->AddGroup(L"Diag >", GroupOpt, &Diagnostics[0]);

    if (Diagnostics[0].Var)
    {
        Menu->AddItem(L"File Logging", ItemOpt, &Diagnostics[1], FUNC_FILELOGGING);
        Menu->AddItem(L"Auto-log files when lagging", ItemOpt, &Diagnostics[2], FUNC_FILEAUTOLOG);
    }

    Menu->AddGroup(L"D3D >", GroupOpt, &D3DTweaks[0]);

    if (D3DTweaks[0].Var)
    {
        Menu->AddItem(L"Windowed", ItemOpt, &D3DTweaks[1], FUNC_WINDOWED);
    }
}


VOID ProcessOptions( MenuItems* Item )
{
    switch (*Item->Id)
    {
    case FUNC_RESET:
        PushSharedMemory->Overloads = 0;
        PushSharedMemory->OSDFlags = 0;
        PushSharedMemory->OSDFlags |= OSD_FPS;
        break;

    case FUNC_FORCEMAX:
        CallPipe(L"ForceMaxClocks", NULL);

        PushSharedMemory->OSDFlags |= OSD_GPU_E_CLK;
        PushSharedMemory->OSDFlags |= OSD_GPU_M_CLK;
        PushSharedMemory->Overloads &= ~OSD_GPU_E_CLK;
        PushSharedMemory->Overloads &= ~OSD_GPU_M_CLK;
        break;

    case FUNC_ECLOCK:
        {
            switch (*Item->Var)
            {
            case 0:
            {
                PushSharedMemory->HarwareInformation.DisplayDevice.EngineClock--;

                UpdateGpuInformation();
                CallPipe(L"UpdateClocks", NULL);
            }
            break;

            case 1:
            {
                PushSharedMemory->HarwareInformation.DisplayDevice.EngineClock++;

                UpdateGpuInformation();
                CallPipe(L"UpdateClocks", NULL);
            }
            break;
            }
        }
        break;

    case FUNC_MCLOCK:
        {
            if (*Item->Var > 0)
            {
                CallPipe(L"Overclock m", NULL);
                UpdateGpuInformation();
            }
        }
        break;

    case FUNC_VOLTAGE:
        CallPipe(L"Overclock v", NULL);
        break;

    case FUNC_FILELOGGING:
        if (*Item->Var > 0)
            PushSharedMemory->LogFileIo = TRUE;
        else
            PushSharedMemory->LogFileIo = FALSE;
        break;

    case FUNC_FILEAUTOLOG:
        if (*Item->Var > 0)
            PushSharedMemory->AutoLogFileIo = TRUE;
        else
            PushSharedMemory->AutoLogFileIo = FALSE;
        break;

    case FUNC_WINDOWED:
        if (*Item->Var > 0)
        {
            D3D9Hook_WindowMode = TRUE;
            D3D9Hook_ForceReset = TRUE;
        }
        else
        {
            D3D9Hook_WindowMode = FALSE;
            D3D9Hook_ForceReset = TRUE;
        }
        break;

    default:
        if (*Item->Var > 0)
            PushSharedMemory->OSDFlags |= *Item->Id;
        else
            PushSharedMemory->OSDFlags &= *Item->Id;
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

WNDPROC         OldWNDPROC;
SlOverlayMenu*  OvmMenu;

VOID D3D9Hook_ApplyHooks();


VOID MenuKeyboardHook( WPARAM Key )
{
    if (Key == VK_INSERT)
    {
        D3D9Hook_ApplyHooks();
    }

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
            Overlay->DrawText(Items[i].Title, X, Y, ColorOne);

        if (Items[i].Type == ITEM)
            Overlay->DrawText(Items[i].Title, X + 20, Y, ColorOne);

        if (Items[i].Options)
            Overlay->DrawText(Items[i].Options[ValueTwo], OpX, Y, ColorTwo);

        Y += HEIGHT;
    }
}

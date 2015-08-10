#include <windows.h>
#include <OvRender.h>
#include <sloverlay.h>
#include <overlay.h>


SlOverlayMenu* Menu;

MenuVars MenuOsd[20];
MenuVars MenuGpu[10];

WCHAR* GroupOpt[] = {L"", L""};
WCHAR* ItemOpt[] = {L"Off", L"On"};
WCHAR* PressOpt[] = { L">>", L">>" };
WCHAR* GpuSpeedEngineOpt[] = { L"XXX MHz", L"XXX MHz" };
WCHAR* GpuSpeedMemoryOpt[] = { L"XXX MHz", L"XXX MHz" };
WCHAR GpuSpeedEngine[20];
WCHAR GpuSpeedMemory[20];

BOOLEAN MnuInitialized;
HANDLE MenuProcessHeap;

//Add menu items to menu
#include <stdio.h>
#include <wchar.h>


VOID InitGpuSpeed()
{
    swprintf(GpuSpeedEngine, 20, L"%i MHz", PushSharedMemory->HarwareInformation.DisplayDevice.EngineClock);
    GpuSpeedEngineOpt[0] = GpuSpeedEngine;
    GpuSpeedEngineOpt[1] = GpuSpeedEngine;

    swprintf(GpuSpeedMemory, 20, L"%i MHz", PushSharedMemory->HarwareInformation.DisplayDevice.MemoryClock);
    GpuSpeedMemoryOpt[0] = GpuSpeedMemory;
    GpuSpeedMemoryOpt[1] = GpuSpeedMemory;
}


VOID AddItems()
{
    Menu->AddGroup(L"OSD", GroupOpt, &MenuOsd[0]);

    if (MenuOsd[0].Var)
    {
        Menu->AddItem(L"GPU Core utilization",    ItemOpt, &MenuOsd[1]);
        Menu->AddItem(L"GPU Core temperature",    ItemOpt, &MenuOsd[2]);
        Menu->AddItem(L"GPU Engine Clock",        ItemOpt, &MenuOsd[3]);
        Menu->AddItem(L"GPU Memory Clock",        ItemOpt, &MenuOsd[4]);
        Menu->AddItem(L"GPU VRAM usage",          ItemOpt, &MenuOsd[5]);
        Menu->AddItem(L"CPU utilization",         ItemOpt, &MenuOsd[6]);
        Menu->AddItem(L"CPU temperature",         ItemOpt, &MenuOsd[7]);
        Menu->AddItem(L"RAM usage",               ItemOpt, &MenuOsd[8]);
        Menu->AddItem(L"Max core usage",          ItemOpt, &MenuOsd[9]);
        Menu->AddItem(L"Max thread usage",        ItemOpt, &MenuOsd[10]);
        Menu->AddItem(L"Disk read-write rate",    ItemOpt, &MenuOsd[11]);
        Menu->AddItem(L"Disk response time",      ItemOpt, &MenuOsd[12]);
        Menu->AddItem(L"Frame Buffer count",      ItemOpt, &MenuOsd[13]);
        Menu->AddItem(L"Show Time",               ItemOpt, &MenuOsd[14]);
        Menu->AddItem(L"Reset Overloads",         PressOpt, &MenuOsd[15]);
    }

    Menu->AddGroup(L"GPU", GroupOpt, &MenuGpu[0]);

    if (MenuGpu[0].Var)
    {
        Menu->AddItem(L"Force Max Clocks", ItemOpt, &MenuGpu[1]);
        Menu->AddItem(L"Engine Clock", GpuSpeedEngineOpt, &MenuGpu[2]);
        Menu->AddItem(L"Memory Clock", GpuSpeedMemoryOpt, &MenuGpu[3]);
        Menu->AddItem(L"NULL", ItemOpt, &MenuGpu[4]);

        //Init gpu clocks
        InitGpuSpeed();
    }
}


VOID CallPipe( WCHAR* Command )
{
    HANDLE hPipe;
    DWORD dwWritten;

    hPipe = CreateFile(TEXT("\\\\.\\pipe\\Push"),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (hPipe != INVALID_HANDLE_VALUE)
    {
        WriteFile(hPipe, Command, (wcslen(Command) + 1) * sizeof(WCHAR), &dwWritten, NULL);
        CloseHandle(hPipe);
    }
}


VOID ProcessOptions()
{
    if (MenuOsd[1].Var > 0)
        PushSharedMemory->OSDFlags |= OSD_GPU_LOAD;
    else
        PushSharedMemory->OSDFlags &= ~OSD_GPU_LOAD;

    if (MenuOsd[2].Var > 0)
        PushSharedMemory->OSDFlags |= OSD_GPU_TEMP;
    else
        PushSharedMemory->OSDFlags &= ~OSD_GPU_TEMP;

    if (MenuOsd[3].Var > 0)
        PushSharedMemory->OSDFlags |= OSD_GPU_E_CLK;
    else
        PushSharedMemory->OSDFlags &= ~OSD_GPU_E_CLK;

    if (MenuOsd[4].Var > 0)
        PushSharedMemory->OSDFlags |= OSD_GPU_M_CLK;
    else
        PushSharedMemory->OSDFlags &= ~OSD_GPU_M_CLK;

    if (MenuOsd[7].Var > 0)
        PushSharedMemory->OSDFlags |= OSD_CPU_TEMP;
    else
        PushSharedMemory->OSDFlags &= ~OSD_CPU_TEMP;

    if (MenuOsd[8].Var > 0)
        PushSharedMemory->OSDFlags |= OSD_RAM;
    else
        PushSharedMemory->OSDFlags &= ~OSD_RAM;
        
    if (MenuOsd[9].Var > 0)
        PushSharedMemory->OSDFlags |= OSD_MCU;
    else
        PushSharedMemory->OSDFlags &= ~OSD_MCU;

    if (MenuOsd[10].Var > 0)
        PushSharedMemory->OSDFlags |= OSD_MTU;
    else
        PushSharedMemory->OSDFlags &= ~OSD_MTU;

    if (MenuOsd[11].Var > 0)
        PushSharedMemory->OSDFlags |= OSD_DISK_RWRATE;
    else
        PushSharedMemory->OSDFlags &= ~OSD_DISK_RWRATE;

    if (MenuOsd[12].Var > 0)
        PushSharedMemory->OSDFlags |= OSD_DISK_RESPONSE;
    else
        PushSharedMemory->OSDFlags &= ~OSD_DISK_RESPONSE;

    if (MenuOsd[14].Var > 0)
        PushSharedMemory->OSDFlags |= OSD_TIME;
    else
        PushSharedMemory->OSDFlags &= ~OSD_TIME;

    if (MenuOsd[15].Var > 0)
    {
        PushSharedMemory->Overloads = 0;
        MenuOsd[15].Var = 0;
    }

    if (MenuGpu[1].Var > 0 && MenuGpu[1].Dirty)
    {
        MenuGpu[1].Dirty = FALSE;

        CallPipe(L"ForceMaxClocks");

        PushSharedMemory->OSDFlags |= OSD_GPU_E_CLK;
        PushSharedMemory->OSDFlags |= OSD_GPU_M_CLK;
        PushSharedMemory->Overloads &= ~OSD_GPU_E_CLK;
        PushSharedMemory->Overloads &= ~OSD_GPU_M_CLK;
    }

    if (MenuGpu[2].Dirty)
    {
        MenuGpu[2].Dirty = FALSE;

        switch (MenuGpu[2].Var)
        {
        case 0:
            CallPipe(L"Overclock e d");
            break;
        case 1:
            CallPipe(L"Overclock e i");
            break;
        }

        InitGpuSpeed();
    }
    

    if (MenuGpu[3].Var > 0 && MenuGpu[3].Dirty)
    {
        MenuGpu[3].Dirty = FALSE;

        CallPipe(L"Overclock m");
        InitGpuSpeed();
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

            *OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Dirty = TRUE;

            if (OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Var && *OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Var > 0)
            {
                *OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Var += -1;

                if (OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Type == GROUP)
                    OvmMenu->mSet.MaxItems = 0;
            }

        } break;

        case VK_RIGHT:
        {
            if (!OvmMenu->mSet.Show)
                break;

            *OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Dirty = TRUE;

            if (OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Var
                && *OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Var < (OvmMenu->Items[OvmMenu->mSet.SeletedIndex].MaxValue - 1))
            {
                *OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Var += 1;

                if (OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Type == GROUP)
                    OvmMenu->mSet.MaxItems = 0;
            }


        } break;
        }

        // Update osd items.
        ProcessOptions();
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
    Items[mSet.MaxItems].Dirty = &Variables->Dirty;
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

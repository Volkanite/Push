#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audiopolicy.h>
#include <OvRender.h>
#include <sloverlay.h>
#include <overlay.h>

#include "kbhook.h"
#include "osd.h"
#include "menu.h"


OverlayMenu* Menu;
OverlayMenu* OvmMenu;

MenuVars MenuOsd[30];
MenuVars MenuGpu[10];
MenuVars Diagnostics[10];
MenuVars D3DTweaks[10];
MenuVars Process[5];
MenuVars Capture[5];
MenuVars Settings[10];

#define GROUP 1
#define ITEM  2
#define HEIGHT 16


DWORD LightBlue = 0xFF4DD0EB;
DWORD Green = 0xFF33FF00;
DWORD White = 0xFFE6E6E6;
DWORD Blue = 0xFF00A4C5;

WCHAR* GroupOpt[] = {L">", L"<"};
WCHAR* ItemOpt[] = {L"Off", L"On"};
WCHAR* ItemOptExt[] = { L"Off", L"On", L"Avg"};
WCHAR* PressOpt[] = { L">>", L">>" };

WCHAR* FontOpt[] = { L"Verdana", L"Consolas", L"Courier", L"DejaVu Sans Mono", L"DFKai-SB", L"DotumChe", L"FangSong",
                     L"GulimChe", L"GungsuhChe", L"KaiTi", L"Liberation Mono", L"Lucida Console", L"MingLiU", L"Miriam Fixed", 
                     L"MS Gothic", L"MS Mincho", L"NSimSun", L"Rod", L"SimHei", L"Simplified Arabic Fixed", L"SimSun", 
                     L"Source Code Pro", L"Unispace" };

WCHAR* CPUCalcOpt[] = { L"twc", L"t", L"o", L"c" };

BOOLEAN MnuInitialized;
HANDLE MenuProcessHeap;

BOOLEAN FontBold;
UINT32 FontSize;

IAudioEndpointVolume    *EndpointVolume;
ISimpleAudioVolume      *SessionVolume;
IAudioSessionEnumerator *AudioSessionEnumerator;
int AmbientVolumeLevel = 100;
extern CPU_CALC_INDEX CPUStrap;

extern OV_WINDOW_MODE D3D9Hook_WindowMode;
extern BOOLEAN D3D9Hook_ForceReset;
extern BOOLEAN DisableAutoOverclock;
extern UINT8 FrameLimit;
extern OSD_VARS Variables;
extern BOOLEAN TakeScreenShot;
extern BOOLEAN StartRecording;
extern BOOLEAN StopRecording;
extern BOOLEAN StripWaitCycles;


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
#define ID_GAPI             OSD_LAST_ITEM+17
#define ID_FRAMETIME        OSD_LAST_ITEM+18
#define ID_SCREENSHOT       OSD_LAST_ITEM+19
#define ID_RECORD           OSD_LAST_ITEM+20
#define ID_FONT             OSD_LAST_ITEM+21
#define ID_BOLD             OSD_LAST_ITEM+22
#define ID_SIZE             OSD_LAST_ITEM+23
#define ID_MVOLUME          OSD_LAST_ITEM+24
#define ID_GVOLUME          OSD_LAST_ITEM+25
#define ID_AVOLUME          OSD_LAST_ITEM+26
#define ID_CPUSTRAP         OSD_LAST_ITEM+27
#define ID_IAPI             OSD_LAST_ITEM+28
#define ID_BUFFERS          OSD_LAST_ITEM+29
#define ID_RESOLUTION       OSD_LAST_ITEM+30


#include <stdio.h>
#include <wchar.h>
#include <math.h>

VOID ChangeVsync(BOOLEAN Setting);
VOID SetFont(WCHAR* FontName, BOOLEAN Bold, UINT32 Size);


VOID UpdateIntegralText( UINT32 Value, WCHAR** OptBuffer )
{
    swprintf(OptBuffer[0], 20, L"%i", Value);
}


VOID AddItems()
{
    Menu->AddGroup(L"OSD", GroupOpt, &MenuOsd[0]);

    if (MenuOsd[0].Var)
    {
        UINT8 i = 1;
        OSD_ITEM *osdItem;

        // Add OSD items to menu.
        osdItem = (OSD_ITEM*)PushSharedMemory->OsdItems;

        for (i = 0; i < PushSharedMemory->NumberOfOsdItems; i++, osdItem++)
        {
            //Skip some items that are handled in another sub-menu
            if (osdItem->Flag == OSD_GPU_E_CLK ||
                osdItem->Flag == OSD_GPU_M_CLK ||
                osdItem->Flag == OSD_GPU_VOLTAGE ||
                osdItem->Flag == OSD_GPU_FAN_DC
                )
            {
                continue;
            }

            Menu->AddItem(osdItem->Description, ItemOpt, &MenuOsd[i+1], osdItem->Flag);
        }

        i++;

        Menu->AddItem(L"FPS", ItemOpt, &MenuOsd[i++], ID_KEEP_FPS);
        Menu->AddItem(L"Reset", PressOpt, &MenuOsd[i++], ID_RESET);
    }

    Menu->AddGroup(L"GPU", GroupOpt, &MenuGpu[0]);

    if (MenuGpu[0].Var)
    {
        Menu->AddItem(L"Force Max Clocks", ItemOpt, &MenuGpu[1], ID_FORCEMAX);
        Menu->AddItem(L"Auto-Overclock", ItemOpt, &MenuGpu[2], ID_OC);
        Menu->AddItem(L"Engine Clock", NULL, &MenuGpu[3], ID_ECLOCK, 1);
        Menu->AddItem(L"Memory Clock", NULL, &MenuGpu[4], ID_MCLOCK, 1);
        Menu->AddItem(L"Voltage", NULL, &MenuGpu[5], ID_VOLTAGE, 1);
        Menu->AddItem(L"Fan Duty Cycle", NULL, &MenuGpu[6], ID_FAN, 1);
    }

    Menu->AddGroup(L"Diagnostics", GroupOpt, &Diagnostics[0]);

    if (Diagnostics[0].Var)
    {
        Menu->AddItem(L"File Logging", ItemOpt, &Diagnostics[1], ID_FILELOGGING);
        Menu->AddItem(L"Auto-log", ItemOpt, &Diagnostics[2], ID_FILEAUTOLOG);
        Menu->AddItem(L"CPU strap", CPUCalcOpt, &Diagnostics[3], ID_CPUSTRAP, 4);
        Menu->AddItem(L"Frame Time", ItemOptExt, &Diagnostics[4], ID_FRAMETIME, 3);
        Menu->AddItem(L"Graphics API", ItemOpt, &Diagnostics[5], ID_GAPI);
        Menu->AddItem(L"Input API", ItemOpt, &Diagnostics[6], ID_IAPI);
        Menu->AddItem(L"Frame Buffer count", ItemOpt, &Diagnostics[7], ID_BUFFERS);
        Menu->AddItem(L"Resolution", ItemOpt, &Diagnostics[8], ID_RESOLUTION);
    }

    Menu->AddGroup(L"Direct3D", GroupOpt, &D3DTweaks[0]);

    if (D3DTweaks[0].Var)
    {
        Menu->AddItem(L"Windowed", ItemOpt, &D3DTweaks[1], ID_WINDOWED);
        Menu->AddItem(L"Keep active", ItemOpt, &D3DTweaks[2], ID_KEEPACTIVE);
        Menu->AddItem(L"Frame Limiter", ItemOpt, &D3DTweaks[3], ID_FRAMELIMITER);
        Menu->AddItem(L"Frame Limit", NULL, &D3DTweaks[4], ID_FRAMELIMIT, 1);
        Menu->AddItem(L"Vsync", ItemOpt, &D3DTweaks[5], ID_VSYNC);
    }

    Menu->AddGroup(L"Process", GroupOpt, &Process[0]);

    if (Process[0].Var)
    {
        Menu->AddItem(L"Terminate", ItemOpt, &Process[1], ID_TERMINATE);
    }

    Menu->AddGroup(L"Capture", GroupOpt, &Capture[0]);

    if (Capture[0].Var)
    {
        Menu->AddItem(L"Screenshot", PressOpt, &Capture[1], ID_SCREENSHOT);
        Menu->AddItem(L"Record", ItemOpt, &Capture[2], ID_RECORD);
    }

    Menu->AddGroup(L"Settings", GroupOpt, &Settings[0]);

    if (Settings[0].Var)
    {
        FontSize = 10;

        Menu->AddItem(L"Font", FontOpt, &Settings[1], ID_FONT, sizeof(FontOpt)/sizeof(FontOpt[0]));
        Menu->AddItem(L"Bold", ItemOpt, &Settings[2], ID_BOLD);
        Menu->AddItem(L"Size", NULL, &Settings[3], ID_SIZE, 1);
        Menu->AddItem(L"Master Volume", NULL, &Settings[4], ID_MVOLUME, 1);
        Menu->AddItem(L"Game Volume", NULL, &Settings[5], ID_GVOLUME, 1);
        Menu->AddItem(L"Ambient Volume", NULL, &Settings[6], ID_AVOLUME, 1);

        //Initialize with current settings
        FontBold = PushSharedMemory->FontBold;
        
        if (FontBold)
            Settings[2].Var = 1;
    }
}


VOID ClockStep( OVERCLOCK_UNIT Unit, CLOCK_STEP_DIRECTION Direction )
{
    UINT32 *value;

    switch (Unit)
    {
    case OC_ENGINE:
        value = &PushSharedMemory->HarwareInformation.DisplayDevice.EngineClockMax;
        PushSharedMemory->OSDFlags |= OSD_GPU_E_CLK;
        break;
    case OC_MEMORY:
        value = &PushSharedMemory->HarwareInformation.DisplayDevice.MemoryClockMax;
        PushSharedMemory->OSDFlags |= OSD_GPU_M_CLK;
        break;
    case OC_VOLTAGE:
        value = &PushSharedMemory->HarwareInformation.DisplayDevice.VoltageMax;
        PushSharedMemory->OSDFlags |= OSD_GPU_VOLTAGE;
        PushSharedMemory->OSDFlags |= OSD_GPU_TEMP;
    default:
        break;
    }

    if (*value == 0)
    {
        return;
    }

    if (Direction == Up)
        (*value)++;
    else if (Direction == Down)
        (*value)--;

    CallPipe(L"UpdateClocks", NULL);
}


void InitializeVolumeManager()
{
    CoInitialize(NULL);
    IMMDeviceEnumerator *deviceEnumerator = NULL;
    HRESULT hr;
    
    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator), 
        NULL, 
        CLSCTX_INPROC_SERVER, 
        __uuidof(IMMDeviceEnumerator), 
        (LPVOID *)&deviceEnumerator
        );
        
    IMMDevice *defaultDevice = NULL;

    hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
    deviceEnumerator->Release();
    deviceEnumerator = NULL;

    hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&EndpointVolume);

    //-------------------------
    IAudioSessionManager2 *pAudioSessionManager2;
    defaultDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, NULL, (VOID**)&pAudioSessionManager2);
    //-------------------------

    defaultDevice->Release();
    defaultDevice = NULL;

    //-------------------------

    pAudioSessionManager2->GetSessionEnumerator(&AudioSessionEnumerator);

    INT nSessionCount;
    AudioSessionEnumerator->GetCount(&nSessionCount);

    DWORD thisPID = GetCurrentProcessId();

    for (INT nSessionIndex = 0; nSessionIndex < nSessionCount; nSessionIndex++)
    {
        IAudioSessionControl *pSessionControl;
        IAudioSessionControl2 *pSessionControl2;

        if (FAILED(AudioSessionEnumerator->GetSession(nSessionIndex, &pSessionControl)))
            continue;

        // Get the extended session control interface pointer.
        pSessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&pSessionControl2);

        DWORD sessionProcessId;
        pSessionControl2->GetProcessId(&sessionProcessId);
        
        if (sessionProcessId == thisPID)
        {
            hr = pSessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&SessionVolume);
        }

        // Clean up.
        pSessionControl2->Release();
        pSessionControl2 = NULL;

        pSessionControl->Release();
        pSessionControl = NULL;
    }
}


int GetVolume()
{
    float level = 0;

    EndpointVolume->GetMasterVolumeLevelScalar(&level);
    
    level *= 100.0f;
    level = roundf(level);

    return level;
}


void SetVolume( int Level )
{
    EndpointVolume->SetMasterVolumeLevelScalar((float)Level / 100.0f, NULL);
}


int GetSessionVolume()
{
    float level = 0;

    SessionVolume->GetMasterVolume(&level);

    level *= 100.0f;
    level = roundf(level);

    return level;
}


void SetSessionVolume( int Level )
{
    SessionVolume->SetMasterVolume((float)Level / 100.0f, NULL);
}


void SetAmbientVolume( int Level )
{
    INT nSessionCount;
    
    AudioSessionEnumerator->GetCount(&nSessionCount);

    DWORD thisPID = GetCurrentProcessId();

    for (INT nSessionIndex = 0; nSessionIndex < nSessionCount; nSessionIndex++)
    {
        IAudioSessionControl *pSessionControl;
        IAudioSessionControl2 *pSessionControl2;

        if (FAILED(AudioSessionEnumerator->GetSession(nSessionIndex, &pSessionControl)))
            continue;

        // Get the extended session control interface pointer.
        pSessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&pSessionControl2);

        DWORD sessionProcessId;
        pSessionControl2->GetProcessId(&sessionProcessId);

        if (sessionProcessId != thisPID)
        {
            ISimpleAudioVolume *sessionVolume;

            pSessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&sessionVolume);
            sessionVolume->SetMasterVolume((float)Level / 100.0f, NULL);
        }

        // Clean up.
        pSessionControl2->Release();
        pSessionControl2 = NULL;

        pSessionControl->Release();
        pSessionControl = NULL;
    }
}


VOID ProcessOptions( MenuItems* Item, WPARAM Key )
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
            switch (Key)
            {
            case VK_LEFT:
            case VK_RIGHT:
                DisableAutoOverclock = TRUE;

                if (Key == VK_RIGHT)
                {
                    ClockStep(OC_ENGINE, Up);
                }
                else if (Key == VK_LEFT)
                {
                    ClockStep(OC_ENGINE, Down);
                }

                UpdateIntegralText(PushSharedMemory->HarwareInformation.DisplayDevice.EngineClockMax, Item->Options);
                break;
            case VK_RETURN:
                PushSharedMemory->OSDFlags |= OSD_GPU_E_CLK;
                break;
            }
        }
        break;

    case ID_MCLOCK:
        {
            switch (Key)
            {
            case VK_LEFT:
            case VK_RIGHT:
                DisableAutoOverclock = TRUE;

                if (Key == VK_RIGHT)
                {
                    ClockStep(OC_MEMORY, Up);
                }
                else if (Key == VK_LEFT)
                {
                    ClockStep(OC_MEMORY, Down);
                }

                UpdateIntegralText(PushSharedMemory->HarwareInformation.DisplayDevice.MemoryClockMax, Item->Options);
                break;
            case VK_RETURN:
                PushSharedMemory->OSDFlags |= OSD_GPU_M_CLK;
                break;
            }
        }
        break;

    case ID_VOLTAGE:
    {
        switch (Key)
        {
        case VK_RIGHT:
            ClockStep(OC_VOLTAGE, Up);
            break;
        case VK_LEFT:
            ClockStep(OC_VOLTAGE, Down);
            break;
        case VK_RETURN:
            PushSharedMemory->OSDFlags |= OSD_GPU_VOLTAGE;
            break;
        }

        UpdateIntegralText(PushSharedMemory->HarwareInformation.DisplayDevice.VoltageMax, Item->Options);
    }
    break;

    case ID_FAN:
    {
        switch (Key)
        {
        case VK_RIGHT:
            PushSharedMemory->HarwareInformation.DisplayDevice.FanDutyCycle++;
            break;
        case VK_LEFT:
            PushSharedMemory->HarwareInformation.DisplayDevice.FanDutyCycle--;
            break;
        case VK_RETURN:
            PushSharedMemory->OSDFlags |= OSD_GPU_FAN_DC;
            break;
        }

        if (Key == VK_LEFT || Key == VK_RIGHT)
        {
            PushSharedMemory->OSDFlags |= OSD_GPU_FAN_DC;
            CallPipe(L"UpdateFanSpeed", NULL);
            UpdateIntegralText(PushSharedMemory->HarwareInformation.DisplayDevice.FanDutyCycle, Item->Options);
        }
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

    case ID_CPUSTRAP:
        if (*Item->Var > 0)
        {
            StripWaitCycles = FALSE;
            int cs = CPUStrap;
            cs++;
            CPUStrap = (CPU_CALC_INDEX) cs;
        }
        else
        {
            int cs = CPUStrap;
            cs--;
            CPUStrap = (CPU_CALC_INDEX)cs;
            StripWaitCycles = TRUE;
        }
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
            if (*Item->Var > 0)
            {
                FrameLimit++;
            }
            else
            {
                FrameLimit--;
            }
            
            UpdateIntegralText(FrameLimit, Item->Options);
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

    case ID_SCREENSHOT:
        if (*Item->Var > 0)
        {
            TakeScreenShot = TRUE;
        }
        break;

    case ID_RECORD:
        if (*Item->Var > 0)
        {
            StartRecording = TRUE;
        }
        else
        {
            StopRecording = TRUE;
        }
        break;

    case ID_FRAMETIME:
        Variables.FrameTime = *Item->Var;
        break;

    case ID_GAPI:
        if (*Item->Var > 0)
            Variables.GraphicsApi = TRUE;
        else
            Variables.GraphicsApi = FALSE;
        break;

    case ID_IAPI:
        if (*Item->Var > 0)
            Variables.InputApi = TRUE;
        else
            Variables.InputApi = FALSE;
        break;

    case ID_RESOLUTION:
        if (*Item->Var > 0)
            Variables.Resolution = TRUE;
        else
            Variables.Resolution = FALSE;
        break;

    case ID_BUFFERS:
        if (*Item->Var > 0)
            Variables.Buffers = TRUE;
        else
            Variables.Buffers = FALSE;
        break;

    case ID_TERMINATE:
        if (*Item->Var > 0)
        {
            TerminateProcess(GetCurrentProcess(), 0);
        }
        break;

    case ID_BOLD:
    case ID_SIZE:
        if (*Item->Id == ID_BOLD)
        {
            if (*Item->Var > 0)
                FontBold = TRUE;
            else
                FontBold = FALSE;
        }

        if (*Item->Id == ID_SIZE)
        {
            if (*Item->Var > 0)
                FontSize++;
            else
                FontSize--;

            UpdateIntegralText(FontSize, Item->Options);
        }

        SetFont(NULL, FontBold, FontSize);
        break;

    case ID_FONT:
        SetFont(FontOpt[*Item->Var], FontBold, FontSize);
        break;

    case ID_MVOLUME:
    {
        int currentVolume = 0;

        currentVolume = GetVolume();

        if (*Item->Var > 0)
        {
            currentVolume++;
        }
        else
        {
            currentVolume--;
        }

        SetVolume(currentVolume);
        UpdateIntegralText(currentVolume, Item->Options);
    }
    break;
    case ID_GVOLUME:
    {
        int currentVolume = 0;

        currentVolume = GetSessionVolume();

        if (*Item->Var > 0)
        {
            currentVolume++;
        }
        else
        {
            currentVolume--;
        }

        SetSessionVolume(currentVolume);
        UpdateIntegralText(currentVolume, Item->Options);
    }
    break;
    case ID_AVOLUME:
    {
        if (*Item->Var > 0)
        {
            AmbientVolumeLevel++;
        }
        else
        {
            AmbientVolumeLevel--;
        }

        SetAmbientVolume(AmbientVolumeLevel);
        UpdateIntegralText(AmbientVolumeLevel, Item->Options);
    }
    break;

    default:
        if (*Item->Id <= OSD_LAST_ITEM)
        {
            if (*Item->Var > 0)
                PushSharedMemory->OSDFlags |= *Item->Id;
            else
                PushSharedMemory->OSDFlags &= ~*Item->Id;
        }
        break;
    }
}


VOID Menu_Render( OvOverlay* Overlay )
{
    if (!MnuInitialized)
    {
        Menu = new OverlayMenu(300);
        MnuInitialized = TRUE;
    }

    if( Menu->mSet.MaxItems == 0 )
        AddItems();

    //Call drawing and navigation functions
    Menu->Render(100, 200, Overlay);
}


VOID Menu_KeyboardHook( WPARAM Key )
{
    if (!OvmMenu)
    {
        return;
    }

    if (Key != VK_INSERT && Key != VK_UP && Key != VK_DOWN && Key != VK_LEFT && Key != VK_RIGHT && Key != VK_RETURN)
    {
        return;
    }

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
            ProcessOptions(&OvmMenu->Items[OvmMenu->mSet.SeletedIndex], Key);

    }break;

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

        //special case for integral items
        if (OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Var && OvmMenu->Items[OvmMenu->mSet.SeletedIndex].MaxValue == 1)
        {
             if (*OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Var <= (OvmMenu->Items[OvmMenu->mSet.SeletedIndex].MaxValue - 1))
             {
                 *OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Var += 1;
             }
        }

        if (OvmMenu->Items[OvmMenu->mSet.SeletedIndex].Type == ITEM)
             ProcessOptions(&OvmMenu->Items[OvmMenu->mSet.SeletedIndex], Key);

    }break;

    case VK_RETURN:
    {
        ProcessOptions(&OvmMenu->Items[OvmMenu->mSet.SeletedIndex], Key);
    }break;

    }
}


BOOLEAN Menu_IsVisible()
{
    return OvmMenu->mSet.Show;
}


OverlayMenu::OverlayMenu( int OptionsX )
{
    OpX = OptionsX;
    
    mSet.Show = FALSE;
    mSet.MaxItems = 0;
    mSet.SeletedIndex = 0;
    
    OvmMenu = this;
    MenuProcessHeap = GetProcessHeap();
}


WCHAR** AllocateOptionsBuffer()
{
    wchar_t **optBuffer;

    optBuffer = (WCHAR**)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WCHAR**));
    optBuffer[0] = (WCHAR*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WCHAR) * 20);

    return optBuffer;
}


VOID OverlayMenu::AddItemToMenu( WCHAR* Title, WCHAR** Options, MenuVars* Variables, int MaxValue, int Type )
{
    Items[mSet.MaxItems].Title = Title;
    Items[mSet.MaxItems].Options= Options;
    Items[mSet.MaxItems].Var = &Variables->Var;
    Items[mSet.MaxItems].MaxValue = MaxValue;
    Items[mSet.MaxItems].Type = Type;
    Items[mSet.MaxItems].Id = &Variables->Id;

    switch (*Items[mSet.MaxItems].Id)
    {
    case ID_FRAMELIMIT:
        Items[mSet.MaxItems].Options = AllocateOptionsBuffer();
        UpdateIntegralText(FrameLimit, Items[mSet.MaxItems].Options);
        break;
    case ID_ECLOCK:
        Items[mSet.MaxItems].Options = AllocateOptionsBuffer();
        UpdateIntegralText(PushSharedMemory->HarwareInformation.DisplayDevice.EngineClockMax, Items[mSet.MaxItems].Options);
        break;
    case ID_MCLOCK:
        Items[mSet.MaxItems].Options = AllocateOptionsBuffer();
        UpdateIntegralText(PushSharedMemory->HarwareInformation.DisplayDevice.MemoryClockMax, Items[mSet.MaxItems].Options);
        break;
    case ID_VOLTAGE:
        Items[mSet.MaxItems].Options = AllocateOptionsBuffer();
        UpdateIntegralText(PushSharedMemory->HarwareInformation.DisplayDevice.VoltageMax, Items[mSet.MaxItems].Options);
        break;
    case ID_FAN:
        Items[mSet.MaxItems].Options = AllocateOptionsBuffer();
        UpdateIntegralText(PushSharedMemory->HarwareInformation.DisplayDevice.FanDutyCycle, Items[mSet.MaxItems].Options);
        break;
    case ID_SIZE:
        Items[mSet.MaxItems].Options = AllocateOptionsBuffer();
        UpdateIntegralText(FontSize, Items[mSet.MaxItems].Options);
        break;
    case ID_MVOLUME:
        Items[mSet.MaxItems].Options = AllocateOptionsBuffer();
        InitializeVolumeManager();
        UpdateIntegralText(GetVolume(), Items[mSet.MaxItems].Options);
        break;
    case ID_GVOLUME:
        Items[mSet.MaxItems].Options = AllocateOptionsBuffer();
        UpdateIntegralText(GetSessionVolume(), Items[mSet.MaxItems].Options);
        break;
    case ID_AVOLUME:
        Items[mSet.MaxItems].Options = AllocateOptionsBuffer();
        UpdateIntegralText(AmbientVolumeLevel, Items[mSet.MaxItems].Options);
        break;
    default:
        break;
    }

    mSet.MaxItems++;
}


VOID OverlayMenu::Render( int X, int Y, OvOverlay* Overlay )
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

            if (Items[i].MaxValue == 1)
                ValueTwo = 0;

            if (Items[i].Options)
                Overlay->DrawText(Items[i].Options[ValueTwo], OpX, Y, ColorTwo);
        }

        Y += HEIGHT;
    }
}

#include <windows.h>
#include "overlay.h"
#include <OvRender.h>
#include "osd.h"
#include "menu.h"
#include <stdio.h>


BOOLEAN g_SetOSDRefresh = TRUE;
BOOLEAN g_FontInited = FALSE;
BOOLEAN IsStableFramerate;
BOOLEAN IsStableFrametime;
BOOLEAN IsLimitedFrametime;
BOOLEAN DisableAutoOverclock;
UINT16 DiskResponseTime;
UINT64 CyclesWaited;
HANDLE RenderThreadHandle;
UINT32 DisplayFrequency;
double FrameTime;
double FrameTimeAvg;
UINT32 Frames;
UINT32 FrameRate;
UINT8 FrameLimit = 80; //80 fps
int debugInt = 1;

VOID DebugRec();
VOID InitializeKeyboardHook();

double PCFreq = 0.0;
__int64 CounterStart = 0;
typedef enum _OVERCLOCK_UNIT
{
    OC_ENGINE,
    OC_MEMORY

}OVERCLOCK_UNIT;
VOID Overclock(OVERCLOCK_UNIT Unit);
extern "C" LONG __stdcall NtQueryPerformanceCounter(
    LARGE_INTEGER* PerformanceCounter,
    LARGE_INTEGER* PerformanceFrequency
    );
VOID StartCounter()
{
    LARGE_INTEGER perfCount;
    LARGE_INTEGER frequency;

    NtQueryPerformanceCounter(&perfCount, &frequency);

    PCFreq = (double)frequency.QuadPart / 1000.0;;
}


double GetPerformanceCounter()
{
    LARGE_INTEGER li;

    NtQueryPerformanceCounter(&li, NULL);

    return (double)li.QuadPart / PCFreq;
}


BOOLEAN IsGpuLag()
{
    if (PushSharedMemory->HarwareInformation.DisplayDevice.Load > 95)
    {
        return TRUE;
    }
    
    return FALSE;
}

#define TSAMP 60
double garb[TSAMP];
int addint = 0;

double GetAverageFrameTime()
{

    UINT32 i;
    double total = 0;

    for (i = 0; i < TSAMP; i++)
        total += garb[i];

    return total / TSAMP;
}

VOID RunFrameStatistics()
{
    static double newTickCount = 0.0, lastTickCount_FrameLimiter = 0.0,
        oldTick = 0.0, delta = 0.0,
        oldTick2 = 0.0, frameTime = 0.0,
        fps = 0.0, acceptableFrameTime = 0.0, lastTickCount;

    
    static BOOLEAN inited = FALSE;
    static UINT32 frames = 0;

    if (!inited)
    {
        DEVMODE devMode;
        StartCounter();
        inited = TRUE;

        EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode);

        DisplayFrequency = devMode.dmDisplayFrequency;
        acceptableFrameTime = (double)1000 / (double)(devMode.dmDisplayFrequency - 1);

        if (PushSharedMemory->FrameLimit > 1)
            FrameLimit = PushSharedMemory->FrameLimit;

        newTickCount = oldTick = lastTickCount_FrameLimiter = GetPerformanceCounter();
    }

    newTickCount = GetPerformanceCounter();
    delta = newTickCount - oldTick;

    IsLimitedFrametime = FALSE;

    frames++;
    Frames++;

    if (PushSharedMemory->FrameLimit)
    {
        frameTime = newTickCount - lastTickCount_FrameLimiter;
    }
    else
    {
        frameTime = newTickCount - lastTickCount;
    }

    garb[addint] = frameTime;
    addint++;

    if (addint == TSAMP)
        addint = 0;

    FrameTimeAvg = GetAverageFrameTime();

    if (delta > 1000)
    {
        // Every second.

        double dummy = delta / 1000.0f;
        fps         = frames / dummy;
        oldTick = newTickCount;
        frames      = 0;
        g_SetOSDRefresh = TRUE;

        //if (PushSharedMemory->ThreadOptimization || PushSharedMemory->OSDFlags & OSD_MTU)
        //{
            PushRefreshThreadMonitor();

            //if (PushSharedMemory->OSDFlags & OSD_MTU)
            PushSharedMemory->HarwareInformation.Processor.MaxThreadUsage = PushGetMaxThreadUsage();
        //}

        RenderThreadHandle = GetCurrentThread();
    }

    // Simple Diagnostics.

    if (frameTime > acceptableFrameTime)
    {
        if (PushSharedMemory->HarwareInformation.Processor.Load > 95)
            PushSharedMemory->Overloads |= OSD_CPU_LOAD;

        if (PushSharedMemory->HarwareInformation.Processor.MaxCoreUsage > 95)
            PushSharedMemory->Overloads |= OSD_MCU;

        if (PushSharedMemory->HarwareInformation.Processor.MaxThreadUsage > 95)
            PushSharedMemory->Overloads |= OSD_MTU;

        if (IsGpuLag()) PushSharedMemory->Overloads |= OSD_GPU_LOAD;

        if (PushSharedMemory->HarwareInformation.Memory.Used > PushSharedMemory->HarwareInformation.Memory.Total)
            PushSharedMemory->Overloads |= OSD_RAM;

        PushSharedMemory->HarwareInformation.Disk.ResponseTime = DiskResponseTime;

        if (PushSharedMemory->HarwareInformation.Disk.ResponseTime > 4000)
        {
            PushSharedMemory->Overloads |= OSD_DISK_RESPONSE;
            PushSharedMemory->OSDFlags |= OSD_DISK_RESPONSE;
        }

        if (PushSharedMemory->AutoLogFileIo)
            PushSharedMemory->LogFileIo = TRUE;

        if (PushSharedMemory->HarwareInformation.Processor.MaxThreadUsage > 95)
        {
            PushSharedMemory->Overloads |= OSD_MTU;
            PushSharedMemory->OSDFlags |= OSD_MTU;
        }
    }

    if (newTickCount - oldTick2 > 30000)
        //frame rate has been stable for at least 30 seconds.
        IsStableFramerate = TRUE;
    else
        IsStableFramerate = FALSE;

    if (FrameTimeAvg > acceptableFrameTime)
    {
        //reset the timer
        oldTick2 = newTickCount;
        IsStableFrametime = FALSE;

        // Lazy overclock
        if (debugInt++ % DisplayFrequency == 0)
        {
            if (!DisableAutoOverclock && IsGpuLag())
            {
                PushSharedMemory->OSDFlags |= OSD_GPU_E_CLK;
                PushSharedMemory->OSDFlags |= OSD_GPU_M_CLK;
                PushSharedMemory->Overloads |= OSD_GPU_E_CLK;
                PushSharedMemory->Overloads |= OSD_GPU_M_CLK;

                if (PushSharedMemory->HarwareInformation.DisplayDevice.EngineClockMax < PushSharedMemory->HarwareInformation.DisplayDevice.EngineOverclock)
                    Overclock(OC_ENGINE);

                if (PushSharedMemory->HarwareInformation.DisplayDevice.MemoryClockMax < PushSharedMemory->HarwareInformation.DisplayDevice.MemoryOverclock)
                    Overclock(OC_MEMORY);
            }
        }
    }
    else
    {
        IsStableFrametime = TRUE;
    }

    if (PushSharedMemory->FrameLimit)
    {
        double frameTimeMin = (double)1000 / (double)FrameLimit;

        if (frameTime < frameTimeMin)
        {
            UINT64 cyclesStart, cyclesStop;

            IsLimitedFrametime = TRUE;

            RenderThreadHandle = GetCurrentThread();

            QueryThreadCycleTime(RenderThreadHandle, &cyclesStart);

            lastTickCount_FrameLimiter = newTickCount;

            while (frameTime < frameTimeMin)
            {
                newTickCount = GetPerformanceCounter();

                frameTime += (newTickCount - lastTickCount_FrameLimiter);

                lastTickCount_FrameLimiter = newTickCount;
            }

            QueryThreadCycleTime(RenderThreadHandle, &cyclesStop);

            CyclesWaited += cyclesStop - cyclesStart;
        }

        lastTickCount_FrameLimiter = newTickCount;
    }

    lastTickCount = newTickCount;
    FrameTime = frameTime;
    FrameRate = (int)fps;
}


VOID RnRender( OvOverlay* Overlay )
{
    static BOOLEAN rendering = FALSE;

    if (!rendering)
    {
        rendering = TRUE;

        InitializeKeyboardHook();
    }

    Osd_Draw( Overlay );
    MnuRender( Overlay );
    RunFrameStatistics();
    
    //Overlay->DrawText(L"u can draw anything in this render loop!\n");
}

#include <windows.h>
#include "overlay.h"
#include <OvRender.h>
#include "osd.h"
#include "menu.h"
#include <stdio.h>


BOOLEAN g_FontInited = FALSE;
BOOLEAN IsStableFramerate;
BOOLEAN IsStableFrametime;
BOOLEAN IsLimitedFrametime;
BOOLEAN DisableAutoOverclock;
UINT16 DiskResponseTime;
UINT64 CyclesWaited;
HANDLE RenderThreadHandle;
UINT32 DisplayFrequency;
double FrameTimeAvg;
double FrameRate;
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

#include <math.h>
bool approximatelyEqual(double a, double b, double epsilon)
{
    return fabs(a - b) < epsilon;
}


VOID RunFrameStatistics()
{
    static double newTickCount = 0.0, lastTickCount_FrameLimiter = 0.0,
        oldTick = 0.0, delta = 0.0,
        oldTick2 = 0.0, frameTime = 0.0, acceptableFrameTime = 0.0, lastTickCount;

    
    static BOOLEAN inited = FALSE;

    if (!inited)
    {
        DEVMODE devMode;
        StartCounter();
        inited = TRUE;

        EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode);

        DisplayFrequency = devMode.dmDisplayFrequency;
        acceptableFrameTime = (double)1000 / (double)(devMode.dmDisplayFrequency - 1);

        if (PushSharedMemory->FrameLimit > 1)
        {
            FrameLimit = PushSharedMemory->FrameLimit;
            acceptableFrameTime = 1000.0f / (double)(FrameLimit - 1);
        }
            
        newTickCount = oldTick = lastTickCount_FrameLimiter = GetPerformanceCounter();
    }

    newTickCount = GetPerformanceCounter();
    delta = newTickCount - oldTick;
    frameTime = newTickCount - lastTickCount;
    lastTickCount = newTickCount;

    IsLimitedFrametime = FALSE;

    garb[addint] = frameTime;
    addint++;

    if (addint == TSAMP)
        addint = 0;

    FrameTimeAvg = GetAverageFrameTime();

    static double OldfFPS = 0.0f;
    FrameRate = 1000.0f / FrameTimeAvg;

    FrameRate = FrameRate * 0.1 + OldfFPS * 0.9;

    OldfFPS = FrameRate;

    // Every second.
    if (delta > 1000)
    {
        oldTick = newTickCount;

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

        if (IsGpuLag()) PushSharedMemory->Overloads |= OSD_GPU_LOAD;

        if (PushSharedMemory->HarwareInformation.Memory.Used > PushSharedMemory->HarwareInformation.Memory.Total)
            PushSharedMemory->Overloads |= OSD_RAM;

        PushSharedMemory->HarwareInformation.Disk.ResponseTime = DiskResponseTime;

        if (PushSharedMemory->HarwareInformation.Disk.ResponseTime > 4000)
        {
            PushSharedMemory->Overloads |= OSD_DISK_RESPONSE;
            PushSharedMemory->OSDFlags |= OSD_DISK_RESPONSE;
        }

        if (PushSharedMemory->HarwareInformation.Processor.MaxThreadUsage >= 99)
        {
            PushSharedMemory->Overloads |= OSD_MTU;
            PushSharedMemory->OSDFlags |= OSD_MTU;
        }

        if (PushSharedMemory->AutoLogFileIo)
            PushSharedMemory->LogFileIo = TRUE;
    }

    //lazier check than (frameTime > acceptableFrameTime)
    if (FrameTimeAvg > acceptableFrameTime)
    {
        //reset the timer
        oldTick2 = newTickCount;

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

    if (newTickCount - oldTick2 > 30000)
        //frame rate has been stable for at least 30 seconds.
        IsStableFramerate = TRUE;
    else
        IsStableFramerate = FALSE;

    double frameTimeDamped = 1000.0f / FrameRate;

    if (frameTimeDamped > acceptableFrameTime)
        IsStableFrametime = FALSE;
    else
        IsStableFrametime = TRUE;

    if (PushSharedMemory->FrameLimit)
    {
        double frameTimeMin = (double)1000 / (double)FrameLimit;

        frameTime = newTickCount - lastTickCount_FrameLimiter;

        if (approximatelyEqual(frameTimeDamped, frameTimeMin, 0.100))
        {
            IsLimitedFrametime = TRUE;
        }

        if (frameTime < frameTimeMin)
        {
            UINT64 cyclesStart, cyclesStop;

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

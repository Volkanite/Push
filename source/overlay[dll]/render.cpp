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

#define TSAMPLES 60
double FrameTimesCircularBuffer[TSAMPLES];
int CircularBufferIndex = 0;

double GetAverageFrameTime()
{

    UINT32 i;
    double total = 0;

    for (i = 0; i < TSAMPLES; i++)
        total += FrameTimesCircularBuffer[i];

    return total / TSAMPLES;
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
        oldTickFrameRateStable = 0.0, frameTime = 0.0, acceptableFrameTime = 0.0, lastTickCount;

    
    static BOOLEAN inited = FALSE;
    static BOOLEAN beginThreadDiagnostics = FALSE;

    if (!inited)
    {
        DEVMODE devMode;
        StartCounter();
        inited = TRUE;

        EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode);

        DisplayFrequency = devMode.dmDisplayFrequency;
        acceptableFrameTime = (double)1000 / (double)(devMode.dmDisplayFrequency - 2);

        if (PushSharedMemory->FrameLimit > 1)
        {
            FrameLimit = PushSharedMemory->FrameLimit;

            if (FrameLimit < DisplayFrequency)
                acceptableFrameTime = 1000.0f / (double)(FrameLimit - 1);
        }
            
        newTickCount = oldTick = lastTickCount_FrameLimiter = GetPerformanceCounter();
    }

    newTickCount = GetPerformanceCounter();
    delta = newTickCount - oldTick;
    frameTime = newTickCount - lastTickCount;
    lastTickCount = newTickCount;

    IsLimitedFrametime = FALSE;

    FrameTimesCircularBuffer[CircularBufferIndex] = frameTime;
    CircularBufferIndex++;

    if (CircularBufferIndex == TSAMPLES)
        CircularBufferIndex = 0;

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

        if (beginThreadDiagnostics && PushSharedMemory->HarwareInformation.Processor.MaxThreadUsage >= 99)
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
        oldTickFrameRateStable = newTickCount;

        // Lazy overclock
        if (debugInt++ % DisplayFrequency == 0)
        {
            if (!DisableAutoOverclock && IsGpuLag() && PushSharedMemory->HarwareInformation.DisplayDevice.EngineClock != 0)
            {
                if (PushSharedMemory->HarwareInformation.DisplayDevice.EngineClockMax < PushSharedMemory->HarwareInformation.DisplayDevice.EngineOverclock)
                {
                    PushSharedMemory->OSDFlags |= OSD_GPU_E_CLK;
                    PushSharedMemory->Overloads |= OSD_GPU_E_CLK;

                    ClockStep(OC_ENGINE, Up);
                }

                if (PushSharedMemory->HarwareInformation.DisplayDevice.MemoryClockMax < PushSharedMemory->HarwareInformation.DisplayDevice.MemoryOverclock)
                {
                    PushSharedMemory->OSDFlags |= OSD_GPU_M_CLK;
                    PushSharedMemory->Overloads |= OSD_GPU_M_CLK;

                    ClockStep(OC_MEMORY, Up);
                }
            }
        }
    }

    if (newTickCount - oldTickFrameRateStable > 30000)
        //frame rate has been stable for at least 30 seconds.
        IsStableFramerate = TRUE;
    else
        IsStableFramerate = FALSE;

    if (newTickCount - oldTickFrameRateStable > 1000)
    {
        //frame rate has been stable for at least 1 second.
        //we put this here to let the game go through it's usual startup routines which normally issues a false positive on thread diagnostics.
        //this tells the diagnostics to hold off until frameTimeAvg (average frame-time) is stable.
        beginThreadDiagnostics = TRUE;
    }

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

        COMMAND_HEADER cmdBuffer;
        cmdBuffer.CommandIndex= CMD_STARTHWMON;
        cmdBuffer.ProcessId = GetCurrentProcessId();
        CallPipe((BYTE*) &cmdBuffer, sizeof(cmdBuffer), NULL);
    }

    Osd_Draw( Overlay );
    Menu_Render( Overlay );
    RunFrameStatistics();
    
    //Overlay->DrawText(L"u can draw anything in this render loop!\n");
}

#include <windows.h>
#include "overlay.h"
#include <OvRender.h>
#include "osd.h"
#include "menu.h"
#include <stdio.h>


UINT8   PushRefreshRate = 60;
UINT8   PushAcceptableFps = 55;
BOOLEAN g_SetOSDRefresh = TRUE;
BOOLEAN g_FontInited = FALSE;
UINT64 g_cyclesWaited = 0;
UINT16 DiskResponseTime;
UINT32 FrameRate;
BOOLEAN IsStableFramerate;
UINT8 FrameLimit = 80; //80 fps


double PCFreq = 0.0;
__int64 CounterStart = 0;


VOID
StartCounter()
{
    LARGE_INTEGER li;
    LARGE_INTEGER freq;

    QueryPerformanceFrequency(&freq);

    PCFreq = (double) freq.QuadPart / 1000.0;

    QueryPerformanceCounter(&li);
    CounterStart = li.QuadPart;
}


double
GetPerformanceCounter()
{
    LARGE_INTEGER li;

    QueryPerformanceCounter(&li);

    return (double) (li.QuadPart - CounterStart) /PCFreq;
}


VOID RunFrameStatistics()
{
    static double newTickCount = 0.0, lastTickCount = 0.0,
                  oldTick = 0.0, delta = 0.0,
                  oldTick2 = 0.0, frameTime = 0.0,
                  fps = 0.0, acceptableFrameTime = 0.0;

    static UINT32 frames = 0;
    static UINT64 lcyclesWaited;
    static BOOLEAN inited = FALSE;

    if (!inited)
    {
      StartCounter();
      inited = TRUE;
      acceptableFrameTime = (double)1000 / (double)PushAcceptableFps;
    }

    newTickCount = GetPerformanceCounter();
    delta = newTickCount - oldTick;
    frameTime = newTickCount- lastTickCount;

    frames++;

    if (delta > 1000)
    {
        // Every second.

        double dummy = delta / 1000.0f;

        fps         = frames / dummy;
        oldTick = newTickCount;
        frames      = 0;
        g_cyclesWaited = lcyclesWaited;
        lcyclesWaited = 0;
        g_SetOSDRefresh = TRUE;

        //if (PushSharedMemory->ThreadOptimization || PushSharedMemory->OSDFlags & OSD_MTU)
        //{
            PushRefreshThreadMonitor();

            //if (PushSharedMemory->OSDFlags & OSD_MTU)
            PushSharedMemory->HarwareInformation.Processor.MaxThreadUsage = PushGetMaxThreadUsage();
        //}

        WCHAR buffer[100];

        swprintf(buffer, 100, L"GetDiskResponseTime %i", GetCurrentProcessId());
        CallPipe(buffer, &DiskResponseTime);
    }

    // Simple Diagnostics.

    if (frameTime > acceptableFrameTime)
    {
        if (PushSharedMemory->HarwareInformation.Processor.Load > 95)
            PushSharedMemory->Overloads |= OSD_CPU_LOAD;

        if (PushSharedMemory->HarwareInformation.Processor.MaxCoreUsage > 95)
            PushSharedMemory->Overloads |= OSD_MCU;

        if (PushSharedMemory->HarwareInformation.DisplayDevice.Load > 95
            && PushSharedMemory->HarwareInformation.DisplayDevice.EngineClock >= PushSharedMemory->HarwareInformation.DisplayDevice.EngineClockMax
            && PushSharedMemory->HarwareInformation.DisplayDevice.MemoryClock >= PushSharedMemory->HarwareInformation.DisplayDevice.MemoryClockMax)
            PushSharedMemory->Overloads |= OSD_GPU_LOAD;

        if (PushSharedMemory->HarwareInformation.Processor.MaxThreadUsage > 95)
            PushSharedMemory->Overloads |= OSD_MTU;

        if ((PushSharedMemory->HarwareInformation.DisplayDevice.EngineClock < PushSharedMemory->HarwareInformation.DisplayDevice.EngineClockMax
            || PushSharedMemory->HarwareInformation.DisplayDevice.MemoryClock < PushSharedMemory->HarwareInformation.DisplayDevice.MemoryClockMax)
            && PushSharedMemory->HarwareInformation.DisplayDevice.Load > 90)
        {
            PushSharedMemory->OSDFlags |= OSD_GPU_E_CLK;
            PushSharedMemory->OSDFlags |= OSD_GPU_M_CLK;
            PushSharedMemory->Overloads |= OSD_GPU_E_CLK;
            PushSharedMemory->Overloads |= OSD_GPU_M_CLK;
        }

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

    if (((newTickCount - oldTick2) > 30000) && !PushSharedMemory->KeepFps)
        //frame rate has been stable for at least 30 seconds, we can disable the thing
        IsStableFramerate = TRUE;
    else
        //you haz really bad frame rates, for you i give an fps meter.
        IsStableFramerate = FALSE;

    if (fps < PushRefreshRate - 1)
    {
        //reset the timer
        oldTick2 = newTickCount;
    }

    FrameRate = (int) fps;

    if (PushSharedMemory->FrameLimit)
    {
        double frameTimeMin = (double)1000 / (double)FrameLimit;

        if (frameTime < frameTimeMin)
        {
            HANDLE threadHandle;
            UINT64 cyclesStart, cyclesStop;

            threadHandle = GetCurrentThread();

            QueryThreadCycleTime(threadHandle, &cyclesStart);

            lastTickCount = newTickCount;

            while (frameTime < frameTimeMin)
            {
                newTickCount = GetPerformanceCounter();

                frameTime += (newTickCount - lastTickCount);

                lastTickCount = newTickCount;
            }

            QueryThreadCycleTime(threadHandle, &cyclesStop);

            lcyclesWaited += cyclesStop - cyclesStart;
        }

        lastTickCount = newTickCount;
    }
}


VOID RnRender( OvOverlay* Overlay )
{
    RunFrameStatistics();
    Osd_Draw( Overlay );
    MnuRender( Overlay );
    //Overlay->DrawText(L"u can draw anything in this render loop!\n");
}

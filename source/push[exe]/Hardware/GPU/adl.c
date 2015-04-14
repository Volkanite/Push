#include <sl.h>

#include "adl.h"
#include "adl_structures.h"


VOID* HeapHandle;
UINT16 EngineClockMaximum;
UINT16 MemoryClockMaximum;
FLOAT VoltageMaximum;
UINT8 PerformanceLevels;


typedef VOID* (__stdcall *TYPE_ADL_Main_Memory_Alloc) ( INT32 );
typedef INT32 (*TYPE_ADL_Main_Control_Create)( TYPE_ADL_Main_Memory_Alloc, INT32 );
typedef INT32 (*TYPE_ADL_Overdrive5_CurrentActivity_Get) ( INT32, ADLPMActivity* );
typedef INT32 (*TYPE_ADL_Overdrive5_Temperature_Get)( INT32, INT32, ADLTemperature* );
typedef INT32 (*TYPE_ADL_Overdrive5_ODParameters_Get)( INT32 iAdapterIndex, ADLODParameters *lpOdParameters );
typedef INT32 (*TYPE_ADL_Overdrive5_ODPerformanceLevels_Get)( INT32 iAdapterIndex, INT32 iDefault, ADLODPerformanceLevels *lpOdPerformanceLevels );
typedef INT32 (*TYPE_ADL_Overdrive5_ODPerformanceLevels_Set )( INT32 iAdapterIndex, ADLODPerformanceLevels *lpOdPerformanceLevels );


TYPE_ADL_Main_Control_Create                ADL_Main_Control_Create;
TYPE_ADL_Overdrive5_CurrentActivity_Get     ADL_Overdrive5_CurrentActivity_Get;
TYPE_ADL_Overdrive5_Temperature_Get         ADL_Overdrive5_Temperature_Get;
TYPE_ADL_Overdrive5_ODParameters_Get        ADL_Overdrive5_ODParameters_Get;
TYPE_ADL_Overdrive5_ODPerformanceLevels_Get ADL_Overdrive5_ODPerformanceLevels_Get;
TYPE_ADL_Overdrive5_ODPerformanceLevels_Set ADL_Overdrive5_ODPerformanceLevels_Set;


VOID* __stdcall ADL_Main_Memory_Alloc( INT32 Size )
{
    VOID *lpBuffer = RtlAllocateHeap(
                        NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap,
                        0,
                        Size
                        );

    return lpBuffer;
}


VOID Adl_Initialize()
{
    VOID *adl = NULL;
    ADLODParameters parameters;
    ADLODPerformanceLevels *performanceLevels;
    UINT8 levels;

    adl = SlLoadLibrary(L"atiadlxy.dll");

    if(!adl)
        return;

    ADL_Main_Control_Create = (TYPE_ADL_Main_Control_Create)
        GetProcAddress(adl, "ADL_Main_Control_Create");

    ADL_Overdrive5_CurrentActivity_Get = (TYPE_ADL_Overdrive5_CurrentActivity_Get)
        GetProcAddress(adl, "ADL_Overdrive5_CurrentActivity_Get");

    ADL_Overdrive5_Temperature_Get = (TYPE_ADL_Overdrive5_Temperature_Get)
        GetProcAddress(adl, "ADL_Overdrive5_Temperature_Get");

    ADL_Overdrive5_ODParameters_Get = (TYPE_ADL_Overdrive5_ODParameters_Get)
        GetProcAddress(adl, "ADL_Overdrive5_ODParameters_Get");

    ADL_Overdrive5_ODPerformanceLevels_Get = (TYPE_ADL_Overdrive5_ODPerformanceLevels_Get)
        GetProcAddress(adl, "ADL_Overdrive5_ODPerformanceLevels_Get");

    ADL_Overdrive5_ODPerformanceLevels_Set = (TYPE_ADL_Overdrive5_ODPerformanceLevels_Set)
        GetProcAddress(adl, "ADL_Overdrive5_ODPerformanceLevels_Set");

    ADL_Main_Control_Create(ADL_Main_Memory_Alloc, 1);

    parameters.Size = sizeof(ADLODParameters);

    ADL_Overdrive5_ODParameters_Get(0, &parameters);

    PerformanceLevels = parameters.NumberOfPerformanceLevels;
    levels = PerformanceLevels - 1;

    HeapHandle = NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap;

    performanceLevels = (ADLODPerformanceLevels*) RtlAllocateHeap(
                            HeapHandle,
                            0,
                            sizeof(ADLODPerformanceLevels) + ( levels * sizeof(ADLODPerformanceLevel) )
                            );

    performanceLevels->Size = sizeof(ADLODPerformanceLevels) + sizeof(ADLODPerformanceLevel) * levels;

    ADL_Overdrive5_ODPerformanceLevels_Get(0, 1, performanceLevels);

    EngineClockMaximum = performanceLevels->Levels[levels].EngineClock / 100;
    MemoryClockMaximum = performanceLevels->Levels[levels].MemoryClock / 100;
    VoltageMaximum = (FLOAT) performanceLevels->Levels[levels].Vddc / 1000;

    RtlFreeHeap(HeapHandle, 0, performanceLevels);
}


UINT8 Adl_GetActivity()
{
    ADLPMActivity activity;

    ADL_Overdrive5_CurrentActivity_Get(0, &activity);

    return activity.ActivityPercent;
}


UINT16 Adl_GetEngineClock()
{
    ADLPMActivity activity;

    ADL_Overdrive5_CurrentActivity_Get(0, &activity);

    return activity.EngineClock / 100;
}


UINT16 Adl_GetMemoryClock()
{
    ADLPMActivity activity;

    ADL_Overdrive5_CurrentActivity_Get(0, &activity);

    return activity.MemoryClock / 100;
}


UINT16 Adl_GetEngineClockMax()
{
    return EngineClockMaximum;
}


UINT16 Adl_GetMemoryClockMax()
{
    return MemoryClockMaximum;
}


VOID Adl_SetEngineClock( UINT16 EngineClock )
{
    ADLODPerformanceLevels *performanceLevels;
    UINT32 i, lev, clock;

    clock = EngineClock * 100;
    lev = PerformanceLevels - 1;

    performanceLevels = (ADLODPerformanceLevels*) RtlAllocateHeap(
                            HeapHandle,
                            0,
                            sizeof(ADLODPerformanceLevels) + (lev * sizeof(ADLODPerformanceLevel))
                            );

    performanceLevels->Size = sizeof(ADLODPerformanceLevels) + sizeof(ADLODPerformanceLevel) * lev;

    ADL_Overdrive5_ODPerformanceLevels_Get(0, 0, performanceLevels);

    for (i = 0; i < lev; i++){
        performanceLevels->Levels[i].EngineClock = clock;
    }

    performanceLevels->Levels[lev].EngineClock = clock;

    ADL_Overdrive5_ODPerformanceLevels_Set(0, performanceLevels);

    RtlFreeHeap(HeapHandle, 0, performanceLevels);
}


VOID Adl_SetMemoryClock( UINT16 MemoryClock )
{
    ADLODPerformanceLevels *performanceLevels;
    UINT32 i, lev, clock;

    clock = MemoryClock * 100;
    lev = PerformanceLevels - 1;

    performanceLevels = (ADLODPerformanceLevels*) RtlAllocateHeap(
                            HeapHandle,
                            0,
                            sizeof(ADLODPerformanceLevels) + (lev * sizeof(ADLODPerformanceLevel))
                            );

    performanceLevels->Size = sizeof(ADLODPerformanceLevels) + sizeof(ADLODPerformanceLevel) * lev;

    ADL_Overdrive5_ODPerformanceLevels_Get(0, 0, performanceLevels);

    for (i = 0; i < lev; i++){
        performanceLevels->Levels[i].MemoryClock = clock;
    }

    performanceLevels->Levels[lev].MemoryClock = clock;

    ADL_Overdrive5_ODPerformanceLevels_Set(0, performanceLevels);

    RtlFreeHeap(HeapHandle, 0, performanceLevels);
}


VOID Adl_SetVoltage( FLOAT Voltage )
{
    ADLODPerformanceLevels *performanceLevels;
    int i, lev, vddc;

    vddc = 1000 * Voltage;
    lev = PerformanceLevels - 1;

    performanceLevels = (ADLODPerformanceLevels*) RtlAllocateHeap(
                            HeapHandle,
                            0,
                            sizeof(ADLODPerformanceLevels) + (lev * sizeof(ADLODPerformanceLevel))
                            );

    performanceLevels->Size = sizeof(ADLODPerformanceLevels) + sizeof(ADLODPerformanceLevel) * lev;

    ADL_Overdrive5_ODPerformanceLevels_Get(0, 0, performanceLevels);

    for (i = 0; i < lev; i++){
        performanceLevels->Levels[i].Vddc = vddc;
    }

    performanceLevels->Levels[lev].Vddc = vddc;

    ADL_Overdrive5_ODPerformanceLevels_Set(0, performanceLevels);

    RtlFreeHeap(HeapHandle, 0, performanceLevels);
}


VOID Adl_SetMaxClocks()
{
    Adl_SetVoltage( VoltageMaximum );
    Adl_SetEngineClock( EngineClockMaximum );
    Adl_SetMemoryClock( MemoryClockMaximum );
}


UINT8 Adl_GetTemperature()
{
    ADLTemperature temperature;

    ADL_Overdrive5_Temperature_Get(0, 0, &temperature);

    return temperature.Temperature / 1000;
}



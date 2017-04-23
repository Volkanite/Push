#include <sl.h>

#include "adl.h"
#include "adl_structures.h"


UINT16 EngineClockMaximum;
UINT16 MemoryClockMaximum;
UINT16 VoltageMaximum;
UINT8 PerformanceLevels;
BOOLEAN AdlInitialized;


typedef VOID* (__stdcall *TYPE_ADL_Main_Memory_Alloc) ( INT32 );
typedef INT32 (*TYPE_ADL_Main_Control_Create)( TYPE_ADL_Main_Memory_Alloc, INT32 );
typedef INT32 (*TYPE_ADL_Overdrive5_CurrentActivity_Get) ( INT32, ADLPMActivity* );
typedef INT32 (*TYPE_ADL_Overdrive5_Temperature_Get)( INT32, INT32, ADLTemperature* );

typedef INT32 (*TYPE_ADL_Overdrive5_ODParameters_Get)(
    INT32 iAdapterIndex,
    ADLODParameters *lpOdParameters
    );

typedef INT32 (*TYPE_ADL_Overdrive5_ODPerformanceLevels_Get)(
    INT32 iAdapterIndex, INT32 iDefault,
    ADLODPerformanceLevels *lpOdPerformanceLevels
    );

typedef INT32 (*TYPE_ADL_Overdrive5_ODPerformanceLevels_Set )(
    INT32 iAdapterIndex,
    ADLODPerformanceLevels *lpOdPerformanceLevels
    );

typedef INT32 (*TYPE_ADL_Overdrive5_FanSpeed_Get)(
    INT32 iAdapterIndex, 
    INT32 iThermalControllerIndex, 
    ADLFanSpeedValue *lpFanSpeedValue
    );


TYPE_ADL_Main_Control_Create                ADL_Main_Control_Create;
TYPE_ADL_Overdrive5_CurrentActivity_Get     ADL_Overdrive5_CurrentActivity_Get;
TYPE_ADL_Overdrive5_Temperature_Get         ADL_Overdrive5_Temperature_Get;
TYPE_ADL_Overdrive5_ODParameters_Get        ADL_Overdrive5_ODParameters_Get;
TYPE_ADL_Overdrive5_ODPerformanceLevels_Get ADL_Overdrive5_ODPerformanceLevels_Get;
TYPE_ADL_Overdrive5_ODPerformanceLevels_Set ADL_Overdrive5_ODPerformanceLevels_Set;
TYPE_ADL_Overdrive5_FanSpeed_Get            ADL_Overdrive5_FanSpeed_Get;


VOID* __stdcall ADL_Main_Memory_Alloc( INT32 Size )
{
    VOID *buffer = Memory_Allocate(Size);

    return buffer;
}


VOID Adl_Initialize()
{
    VOID *adl = NULL;
    ADLODParameters parameters;
    ADLODPerformanceLevels *performanceLevels;
    UINT8 levels;

    adl = Module_Load(L"atiadlxy.dll");

    if (!adl) return;

    AdlInitialized = TRUE;

    ADL_Main_Control_Create = (TYPE_ADL_Main_Control_Create) 
        Module_GetProcedureAddress(adl, "ADL_Main_Control_Create");

    ADL_Overdrive5_CurrentActivity_Get = (TYPE_ADL_Overdrive5_CurrentActivity_Get) 
        Module_GetProcedureAddress(adl, "ADL_Overdrive5_CurrentActivity_Get");

    ADL_Overdrive5_Temperature_Get = (TYPE_ADL_Overdrive5_Temperature_Get) 
        Module_GetProcedureAddress(adl, "ADL_Overdrive5_Temperature_Get");

    ADL_Overdrive5_ODParameters_Get = (TYPE_ADL_Overdrive5_ODParameters_Get) 
        Module_GetProcedureAddress(adl, "ADL_Overdrive5_ODParameters_Get");

    ADL_Overdrive5_ODPerformanceLevels_Get = (TYPE_ADL_Overdrive5_ODPerformanceLevels_Get) 
        Module_GetProcedureAddress(adl, "ADL_Overdrive5_ODPerformanceLevels_Get");

    ADL_Overdrive5_ODPerformanceLevels_Set = (TYPE_ADL_Overdrive5_ODPerformanceLevels_Set) 
        Module_GetProcedureAddress(adl, "ADL_Overdrive5_ODPerformanceLevels_Set");

    ADL_Overdrive5_FanSpeed_Get = (TYPE_ADL_Overdrive5_FanSpeed_Get)
        Module_GetProcedureAddress(adl, "ADL_Overdrive5_FanSpeed_Get");

    ADL_Main_Control_Create(ADL_Main_Memory_Alloc, 1);

    parameters.Size = sizeof(ADLODParameters);

    ADL_Overdrive5_ODParameters_Get(0, &parameters);

    PerformanceLevels = parameters.NumberOfPerformanceLevels;
    levels = PerformanceLevels - 1;

    performanceLevels = (ADLODPerformanceLevels*) Memory_Allocate(
        sizeof(ADLODPerformanceLevels) + ( levels * sizeof(ADLODPerformanceLevel) ) 
        );

    performanceLevels->Size = sizeof(ADLODPerformanceLevels) + sizeof(ADLODPerformanceLevel) * levels;

    ADL_Overdrive5_ODPerformanceLevels_Get(0, 1, performanceLevels);

    EngineClockMaximum = performanceLevels->Levels[levels].EngineClock / 100;
    MemoryClockMaximum = performanceLevels->Levels[levels].MemoryClock / 100;
    VoltageMaximum = performanceLevels->Levels[levels].Vddc;

    Memory_Free(performanceLevels);
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

    if (!AdlInitialized)
        return 0;

    ADL_Overdrive5_CurrentActivity_Get(0, &activity);

    return activity.EngineClock / 100;
}


UINT16 Adl_GetMemoryClock()
{
    ADLPMActivity activity;

    if (!AdlInitialized)
        return 0;

    ADL_Overdrive5_CurrentActivity_Get(0, &activity);

    return activity.MemoryClock / 100;
}


UINT16 Adl_GetEngineClockMax()
{
    ADLODPerformanceLevels *performanceLevels;
    UINT32 lev, clock;

    lev = PerformanceLevels - 1;

    performanceLevels = (ADLODPerformanceLevels*)Memory_Allocate(
        sizeof(ADLODPerformanceLevels) + (lev * sizeof(ADLODPerformanceLevel))
        );

    performanceLevels->Size = sizeof(ADLODPerformanceLevels) + sizeof(ADLODPerformanceLevel) * lev;

    ADL_Overdrive5_ODPerformanceLevels_Get(0, 0, performanceLevels);

    clock = performanceLevels->Levels[lev].EngineClock;

    Memory_Free(performanceLevels);

    return clock / 100;
}


UINT16 Adl_GetMemoryClockMax()
{
    return MemoryClockMaximum;
}


VOID Adl_SetEngineClock( UINT16 EngineClock )
{
    ADLODPerformanceLevels *performanceLevels;
    UINT32 lev, clock;

    clock = EngineClock * 100;
    lev = PerformanceLevels - 1;

    performanceLevels = (ADLODPerformanceLevels*) Memory_Allocate(
        sizeof(ADLODPerformanceLevels) + (lev * sizeof(ADLODPerformanceLevel))
        );

    performanceLevels->Size = sizeof(ADLODPerformanceLevels) + sizeof(ADLODPerformanceLevel) * lev;

    ADL_Overdrive5_ODPerformanceLevels_Get(0, 0, performanceLevels);

    performanceLevels->Levels[lev].EngineClock = clock;

    ADL_Overdrive5_ODPerformanceLevels_Set(0, performanceLevels);

    Memory_Free(performanceLevels);
}


VOID Adl_SetMemoryClock( UINT16 MemoryClock )
{
    ADLODPerformanceLevels *performanceLevels;
    UINT32 lev, clock;

    clock = MemoryClock * 100;
    lev = PerformanceLevels - 1;

    performanceLevels = (ADLODPerformanceLevels*) Memory_Allocate(
        sizeof(ADLODPerformanceLevels) + (lev * sizeof(ADLODPerformanceLevel))
        );

    performanceLevels->Size = sizeof(ADLODPerformanceLevels) + sizeof(ADLODPerformanceLevel) * lev;

    ADL_Overdrive5_ODPerformanceLevels_Get(0, 0, performanceLevels);

    performanceLevels->Levels[lev].MemoryClock = clock;

    ADL_Overdrive5_ODPerformanceLevels_Set(0, performanceLevels);

    Memory_Free(performanceLevels);
}


UINT16 Adl_GetVoltage()
{
    ADLPMActivity activity;

    ADL_Overdrive5_CurrentActivity_Get(0, &activity);

    return activity.iVddc;
}


VOID Adl_SetVoltage( UINT16 Voltage )
{
    ADLODPerformanceLevels *performanceLevels;
    int lev, vddc;

    vddc = Voltage;
    lev = PerformanceLevels - 1;

    performanceLevels = (ADLODPerformanceLevels*) Memory_Allocate(
        sizeof(ADLODPerformanceLevels) + (lev * sizeof(ADLODPerformanceLevel))
        );

    performanceLevels->Size = sizeof(ADLODPerformanceLevels) + sizeof(ADLODPerformanceLevel) * lev;

    ADL_Overdrive5_ODPerformanceLevels_Get(0, 0, performanceLevels);

    performanceLevels->Levels[lev].Vddc = vddc;

    ADL_Overdrive5_ODPerformanceLevels_Set(0, performanceLevels);

    Memory_Free(performanceLevels);
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


UINT32 Adl_GetFanSpeed()
{
    ADLFanSpeedValue fanSpeed;

    fanSpeed.iSpeedType = ADL_DL_FANCTRL_SPEED_TYPE_RPM;

    ADL_Overdrive5_FanSpeed_Get(0, 0, &fanSpeed);

    return fanSpeed.iFanSpeed;
}
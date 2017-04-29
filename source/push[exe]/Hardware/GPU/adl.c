#include <sl.h>

#include "adl.h"
#include "adl_structures.h"
#include "adl_functions.h"


UINT16 EngineClockMaximum;
UINT16 MemoryClockMaximum;
UINT16 VoltageMaximum;
UINT8 PerformanceLevels;
BOOLEAN AdlInitialized;
int FanFlags;


ADL_MAIN_CONTROL_CREATE                 ADL_Main_Control_Create;
ADL_OVERDRIVE5_CURRENTACTIVITY_GET      ADL_Overdrive5_CurrentActivity_Get;
ADL_OVERDRIVE5_TEMPERATURE_GET          ADL_Overdrive5_Temperature_Get;
ADL_OVERDRIVE5_ODPARAMETERS_GET         ADL_Overdrive5_ODParameters_Get;
ADL_OVERDRIVE5_ODPERFORMANCELEVELS_GET  ADL_Overdrive5_ODPerformanceLevels_Get;
ADL_OVERDRIVE5_ODPERFORMANCELEVELS_SET  ADL_Overdrive5_ODPerformanceLevels_Set;
ADL_OVERDRIVE5_FANSPEED_GET             ADL_Overdrive5_FanSpeed_Get;
ADL_OVERDRIVE5_FANSPEEDINFO_GET         ADL_Overdrive5_FanSpeedInfo_Get;


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
    ADLFanSpeedInfo fanSpeedInfo;
    UINT8 levels;

    adl = Module_Load(L"atiadlxy.dll");

    if (!adl) return;

    AdlInitialized = TRUE;

    ADL_Main_Control_Create = (ADL_MAIN_CONTROL_CREATE)
        Module_GetProcedureAddress(adl, "ADL_Main_Control_Create");

    ADL_Overdrive5_CurrentActivity_Get = (ADL_OVERDRIVE5_CURRENTACTIVITY_GET)
        Module_GetProcedureAddress(adl, "ADL_Overdrive5_CurrentActivity_Get");

    ADL_Overdrive5_Temperature_Get = (ADL_OVERDRIVE5_TEMPERATURE_GET)
        Module_GetProcedureAddress(adl, "ADL_Overdrive5_Temperature_Get");

    ADL_Overdrive5_ODParameters_Get = (ADL_OVERDRIVE5_ODPARAMETERS_GET)
        Module_GetProcedureAddress(adl, "ADL_Overdrive5_ODParameters_Get");

    ADL_Overdrive5_ODPerformanceLevels_Get = (ADL_OVERDRIVE5_ODPERFORMANCELEVELS_GET)
        Module_GetProcedureAddress(adl, "ADL_Overdrive5_ODPerformanceLevels_Get");

    ADL_Overdrive5_ODPerformanceLevels_Set = (ADL_OVERDRIVE5_ODPERFORMANCELEVELS_SET)
        Module_GetProcedureAddress(adl, "ADL_Overdrive5_ODPerformanceLevels_Set");

    ADL_Overdrive5_FanSpeed_Get = (ADL_OVERDRIVE5_FANSPEED_GET)
        Module_GetProcedureAddress(adl, "ADL_Overdrive5_FanSpeed_Get");

    ADL_Overdrive5_FanSpeedInfo_Get = (ADL_OVERDRIVE5_FANSPEEDINFO_GET)
        Module_GetProcedureAddress(adl, "ADL_Overdrive5_FanSpeedInfo_Get");

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

    ADL_Overdrive5_FanSpeedInfo_Get(0, 0, &fanSpeedInfo);

    FanFlags = fanSpeedInfo.iFlags;
}


VOID Adl_GetInfo( GPU_INFO* Information )
{
    ADLPMActivity activity;
    ADLTemperature temperature;
    ADLFanSpeedValue fanSpeed;

    ADL_Overdrive5_CurrentActivity_Get(0, &activity);

    Information->Load = activity.ActivityPercent;
    Information->EngineClock = activity.EngineClock / 100;
    Information->MemoryClock = activity.MemoryClock / 100;
    Information->Voltage = activity.iVddc;

    ADL_Overdrive5_Temperature_Get(0, 0, &temperature);

    Information->Temperature = temperature.Temperature / 1000;

    if (FanFlags & ADL_DL_FANCTRL_SUPPORTS_RPM_READ)
    {
        fanSpeed.iSpeedType = ADL_DL_FANCTRL_SPEED_TYPE_RPM;

        int status = ADL_Overdrive5_FanSpeed_Get(0, 0, &fanSpeed);

        Information->FanSpeed = fanSpeed.iFanSpeed;
    }
    else
    {
        Information->FanSpeed = 0;
    }

    if (FanFlags & ADL_DL_FANCTRL_SUPPORTS_PERCENT_READ)
    {
        fanSpeed.iSpeedType = ADL_DL_FANCTRL_SPEED_TYPE_PERCENT;

        int status = ADL_Overdrive5_FanSpeed_Get(0, 0, &fanSpeed);

        Information->FanDutyCycle = fanSpeed.iFanSpeed;
    }
    else
    {
        Information->FanDutyCycle = 0;
    }
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


VOID Adl_SetEngineClock( UINT16 EngineClock, UINT8 PerformanceLevel )
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

    performanceLevels->Levels[PerformanceLevel].EngineClock = clock;

    ADL_Overdrive5_ODPerformanceLevels_Set(0, performanceLevels);

    Memory_Free(performanceLevels);
}


VOID Adl_SetMemoryClock( UINT16 MemoryClock, UINT8 PerformanceLevel )
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

    performanceLevels->Levels[PerformanceLevel].MemoryClock = clock;

    ADL_Overdrive5_ODPerformanceLevels_Set(0, performanceLevels);

    Memory_Free(performanceLevels);
}


UINT16 Adl_GetVoltageMax()
{
    return VoltageMaximum;
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
    Adl_SetEngineClock( EngineClockMaximum, 2);
    Adl_SetMemoryClock( MemoryClockMaximum, 2);
}
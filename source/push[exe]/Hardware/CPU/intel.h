typedef enum _INTEL_SPEED_INDEX
{
    INTEL_SPEED_LFM,
    INTEL_SPEED_HFM,
    INTEL_SPEED_TURBO,
    INTEL_SPEED_STATUS

} INTEL_SPEED_INDEX;


VOID IntelCPU_Initialize();

UINT8 Intel_GetTemperature();
UINT8 Intel_GetTemperatureMaximal();
UINT16 Intel_GetSpeed(INTEL_SPEED_INDEX Index);
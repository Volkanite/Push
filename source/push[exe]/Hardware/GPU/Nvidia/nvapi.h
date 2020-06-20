#define NVAPI_MAX_PHYSICAL_GPUS         64
#define NVAPI_MAX_USAGES_PER_GPU        33
#define NVAPI_MAX_PHYSICAL_GPUS         64
#define NVAPI_MAX_CLOCKS_PER_GPU        0x120
#define NVAPI_MAX_MEMORY_VALUES_PER_GPU 5
#define NVAPI_MAX_GPU_PERF_PSTATES      16
#define NVAPI_MAX_GPU_PUBLIC_CLOCKS     32
#define NVAPI_MAX_GPU_PERF_CLOCKS       32
#define NVAPI_MAX_THERMAL_SENSORS_PER_GPU 3

#pragma pack(push,1)
typedef enum
{
    NVAPI_CLOCK_TARGET_ENGINE = 0,
    NVAPI_CLOCK_TARGET_MEMORY = 8,

} NV_CLOCK_TARGET;

typedef struct _NV_CLOCKS
{
    UINT32 Version;
    UINT32 Clocks[NVAPI_MAX_CLOCKS_PER_GPU];

}NV_CLOCKS;

typedef enum
{
    NVAPI_USAGE_TARGET_GPU = 2,

} NV_USAGE_TARGET;

typedef struct _NV_USAGES
{
    UINT32 Version;
    UINT32 Usage[NVAPI_MAX_USAGES_PER_GPU];

}NV_USAGES;

typedef struct _NV_MEMORY_INFO
{
    UINT32 Version;
    UINT32 Value[NVAPI_MAX_MEMORY_VALUES_PER_GPU];

}NV_MEMORY_INFO;

typedef enum _NV_GPU_PERF_PSTATE_ID
{
    NVAPI_GPU_PERF_PSTATE_P0 = 0,
    NVAPI_GPU_PERF_PSTATE_P1,
    NVAPI_GPU_PERF_PSTATE_P2,
    NVAPI_GPU_PERF_PSTATE_P3,
    NVAPI_GPU_PERF_PSTATE_P4,
    NVAPI_GPU_PERF_PSTATE_P5,
    NVAPI_GPU_PERF_PSTATE_P6,
    NVAPI_GPU_PERF_PSTATE_P7,
    NVAPI_GPU_PERF_PSTATE_P8,
    NVAPI_GPU_PERF_PSTATE_P9,
    NVAPI_GPU_PERF_PSTATE_P10,
    NVAPI_GPU_PERF_PSTATE_P11,
    NVAPI_GPU_PERF_PSTATE_P12,
    NVAPI_GPU_PERF_PSTATE_P13,
    NVAPI_GPU_PERF_PSTATE_P14,
    NVAPI_GPU_PERF_PSTATE_P15,
    NVAPI_GPU_PERF_PSTATE_UNDEFINED = NVAPI_MAX_GPU_PERF_PSTATES,
    NVAPI_GPU_PERF_PSTATE_ALL,

} NV_GPU_PERF_PSTATE_ID;

typedef enum _NV_GPU_PUBLIC_CLOCK_ID
{
	NVAPI_GPU_PUBLIC_CLOCK_GRAPHICS = 0,
	NVAPI_GPU_PUBLIC_CLOCK_MEMORY = 4,
	NVAPI_GPU_PUBLIC_CLOCK_PROCESSOR = 7,
	NVAPI_GPU_PUBLIC_CLOCK_VIDEO = 8,
	NVAPI_GPU_PUBLIC_CLOCK_UNDEFINED = NVAPI_MAX_GPU_PUBLIC_CLOCKS

}NV_GPU_PUBLIC_CLOCK_ID;

typedef struct
{
    UINT32 Version;
    UINT32 Flags;
    UINT32 NumPstates;
    UINT32 NumClocks;

    struct
    {
        NV_GPU_PERF_PSTATE_ID   PstateId;
        UINT32                  Flags;

        struct
        {
            NV_GPU_PUBLIC_CLOCK_ID  DomainId;
            UINT32                  Flags;
            UINT32                  Freq;

        } Clocks[NVAPI_MAX_GPU_PERF_CLOCKS];
    } Pstates[NVAPI_MAX_GPU_PERF_PSTATES];

} NV_GPU_PERF_PSTATES_INFO;

typedef enum
{
    NVAPI_THERMAL_CONTROLLER_NONE = 0,
    NVAPI_THERMAL_CONTROLLER_GPU_INTERNAL,
    NVAPI_THERMAL_CONTROLLER_ADM1032,
    NVAPI_THERMAL_CONTROLLER_MAX6649,
    NVAPI_THERMAL_CONTROLLER_MAX1617,
    NVAPI_THERMAL_CONTROLLER_LM99,
    NVAPI_THERMAL_CONTROLLER_LM89,
    NVAPI_THERMAL_CONTROLLER_LM64,
    NVAPI_THERMAL_CONTROLLER_ADT7473,
    NVAPI_THERMAL_CONTROLLER_SBMAX6649,
    NVAPI_THERMAL_CONTROLLER_VBIOSEVT,
    NVAPI_THERMAL_CONTROLLER_OS,
    NVAPI_THERMAL_CONTROLLER_UNKNOWN = -1,
} NV_THERMAL_CONTROLLER;

typedef enum
{
    NVAPI_THERMAL_TARGET_NONE = 0,
    NVAPI_THERMAL_TARGET_GPU = 1,     //!< GPU core temperature requires NvPhysicalGpuHandle
    NVAPI_THERMAL_TARGET_MEMORY = 2,     //!< GPU memory temperature requires NvPhysicalGpuHandle
    NVAPI_THERMAL_TARGET_POWER_SUPPLY = 4,     //!< GPU power supply temperature requires NvPhysicalGpuHandle
    NVAPI_THERMAL_TARGET_BOARD = 8,     //!< GPU board ambient temperature requires NvPhysicalGpuHandle
    NVAPI_THERMAL_TARGET_VCD_BOARD = 9,     //!< Visual Computing Device Board temperature requires NvVisualComputingDeviceHandle
    NVAPI_THERMAL_TARGET_VCD_INLET = 10,    //!< Visual Computing Device Inlet temperature requires NvVisualComputingDeviceHandle
    NVAPI_THERMAL_TARGET_VCD_OUTLET = 11,    //!< Visual Computing Device Outlet temperature requires NvVisualComputingDeviceHandle

    NVAPI_THERMAL_TARGET_ALL = 15,
    NVAPI_THERMAL_TARGET_UNKNOWN = -1,
} NV_THERMAL_TARGET;

typedef struct
{
    UINT32   Version;                //!< structure version
    UINT32   Count;                  //!< number of associated thermal sensors
    struct
    {
        NV_THERMAL_CONTROLLER       Controller;         //!< internal, ADM1032, MAX6649...
        INT32                       DefaultMinTemp;     //!< Minimum default temperature value of the thermal sensor in degrees C
        INT32                       DefaultMaxTemp;     //!< Maximum default temperature value of the thermal sensor in degrees C
        INT32                       CurrentTemp;        //!< Current temperature value of the thermal sensor in degrees C
        NV_THERMAL_TARGET           Target;             //!< Thermal sensor targeted - GPU, memory, chipset, powersupply, Visual Computing Device, etc
    } sensor[NVAPI_MAX_THERMAL_SENSORS_PER_GPU];

} NV_GPU_THERMAL_SETTINGS;

#define NVAPI_MAX_GPU_PERF_VOLTAGES     16

typedef enum _NV_GPU_PERF_VOLTAGE_INFO_DOMAIN_ID
{
    NVAPI_GPU_PERF_VOLTAGE_INFO_DOMAIN_CORE = 0,
    NVAPI_GPU_PERF_VOLTAGE_INFO_DOMAIN_UNDEFINED = NVAPI_MAX_GPU_PERF_VOLTAGES,
} NV_GPU_PERF_VOLTAGE_INFO_DOMAIN_ID;

typedef struct _NV_VOLTAGES_INFO
{
    NV_GPU_PERF_VOLTAGE_INFO_DOMAIN_ID      domainId;       //!< ID of the voltage domain
    UINT32                                   unknown1;
    UINT32                                   max;

    struct
    {
        UINT32                               unknown2;
        UINT32                               mvolt;          //!< Voltage in mV
    } info[128];
} NV_VOLTAGES_INFO;

typedef struct _NV_VOLTAGES
{
    UINT32                                   Version;        //!< Structure version
    UINT32                                   Flags;          //!< Reserved for future use. Must be set to 0
    UINT32                                   max;
    NV_VOLTAGES_INFO voltages[NVAPI_MAX_GPU_PERF_VOLTAGES];
} NV_VOLTAGES;


typedef struct _NV_GPU_COOLER_SETTINGS_COOLER
{
	INT32 Type;
	INT32 Controller;
	UINT32 DefaultMinLevel;
	UINT32 DefaultMaxLevel;
	UINT32 CurrentMinLevel;
	UINT32 CurrentMaxLevel;
	UINT32 CurrentLevel;
	INT32 DefaultPolicy;
	INT32 CurrentPolicy;
	INT32 Target;
	INT32 ControlType;
	INT32 Active;

}NV_GPU_COOLER_SETTINGS_COOLER;


#define NVAPI_MAX_COOLERS_PER_GPU   20

typedef struct _NV_GPU_COOLER_SETTINGS_V1
{
	UINT32 Version;
	UINT32 Count;
	NV_GPU_COOLER_SETTINGS_COOLER Cooler[NVAPI_MAX_COOLERS_PER_GPU];

}NV_GPU_COOLER_SETTINGS_V1;


/*typedef struct _NV_GPU_CLOCK_FREQUENCIES_V1
{
	UINT32 Version;
	UINT32 	Reserved;

	struct 
	{
		UINT32   bIsPresent : 1;
		UINT32   reserved : 31;
		UINT32   frequency;

	} Domain[NVAPI_MAX_GPU_PUBLIC_CLOCKS];

}NV_GPU_CLOCK_FREQUENCIES_V1;*/

typedef unsigned long NvU32;

typedef struct
{
	NvU32   version;        //!< Structure version
	NvU32   ClockType : 2;    //!< One of NV_GPU_CLOCK_FREQUENCIES_CLOCK_TYPE. Used to specify the type of clock to be returned.
	NvU32   reserved : 22;    //!< These bits are reserved for future use. Must be set to 0.
	NvU32   reserved1 : 8;    //!< These bits are reserved.
	struct
	{
		NvU32 bIsPresent : 1;         //!< Set if this domain is present on this GPU
		NvU32 reserved : 31;          //!< These bits are reserved for future use.
		NvU32 frequency;            //!< Clock frequency (kHz)
	}domain[NVAPI_MAX_GPU_PUBLIC_CLOCKS];
} NV_GPU_CLOCK_FREQUENCIES_V2;


typedef enum _NV_COOLER_POLICY
{
	NVAPI_COOLER_POLICY_NONE = 0,
	NVAPI_COOLER_POLICY_MANUAL,                     // Manual adjustment of cooler level. Gets applied right away independent of temperature or performance level.
	NVAPI_COOLER_POLICY_PERF,                       // GPU performance controls the cooler level.
	NVAPI_COOLER_POLICY_TEMPERATURE_DISCRETE = 4,   // Discrete thermal levels control the cooler level.
	NVAPI_COOLER_POLICY_TEMPERATURE_CONTINUOUS = 8, // Cooler level adjusted at continuous thermal levels.
	NVAPI_COOLER_POLICY_HYBRID,                     // Hybrid of performance and temperature levels.
} NV_COOLER_POLICY;

typedef struct _NV_GPU_SETCOOLER_LEVEL
{
	UINT32 Version;                       //structure version
	struct
	{
		UINT32 CurrentLevel;              // the new value % of the cooler
		NV_COOLER_POLICY CurrentPolicy;  // the new cooler control policy - auto-perf, auto-thermal, manual, hybrid...
	} Cooler[NVAPI_MAX_COOLERS_PER_GPU];
} NV_GPU_SETCOOLER_LEVEL;


typedef struct {
	int value;
	struct {
		int   mindelta;
		int   maxdelta;
	} valueRange;
} NV_GPU_PERF_PSTATES20_PARAM_DELTA;

typedef struct {
	NvU32   domainId;
	NvU32   typeId;
	NvU32   bIsEditable : 1;
	NvU32   reserved : 31;
	NV_GPU_PERF_PSTATES20_PARAM_DELTA   freqDelta_kHz;
	union {
		struct {
			NvU32   freq_kHz;
		} single;
		struct {
			NvU32   minFreq_kHz;
			NvU32   maxFreq_kHz;
			NvU32   domainId;
			NvU32   minVoltage_uV;
			NvU32   maxVoltage_uV;
		} range;
	} data;
} NV_GPU_PSTATE20_CLOCK_ENTRY_V1;

typedef struct {
	NvU32   domainId;
	NvU32   bIsEditable : 1;
	NvU32   reserved : 31;
	NvU32   volt_uV;
	int     voltDelta_uV;
} NV_GPU_PSTATE20_BASE_VOLTAGE_ENTRY_V1;

typedef struct {
	NvU32   version;
	NvU32   bIsEditable : 1;
	NvU32   reserved : 31;
	NvU32   numPstates;
	NvU32   numClocks;
	NvU32   numBaseVoltages;
	struct {
		NvU32                                   pstateId;
		NvU32                                   bIsEditable : 1;
		NvU32                                   reserved : 31;
		NV_GPU_PSTATE20_CLOCK_ENTRY_V1          clocks[8];
		NV_GPU_PSTATE20_BASE_VOLTAGE_ENTRY_V1   baseVoltages[4];
	} pstates[16];
} NV_GPU_PERF_PSTATES20_INFO_V1;
#pragma pack(pop)

typedef enum
{
	NV_GPU_CLOCK_FREQUENCIES_CURRENT_FREQ = 0,
	NV_GPU_CLOCK_FREQUENCIES_BASE_CLOCK = 1,
	NV_GPU_CLOCK_FREQUENCIES_BOOST_CLOCK = 2,
	NV_GPU_CLOCK_FREQUENCIES_CLOCK_TYPE_NUM = 3
}   NV_GPU_CLOCK_FREQUENCIES_CLOCK_TYPE;


typedef INT32 NvAPI_Status;

typedef INT32 *(*TYPE_NvAPI_QueryInterface)(UINT32 offset);
typedef INT32(*TYPE_NvAPI_Initialize)();
typedef NvAPI_Status(*TYPE_NvAPI_EnumPhysicalGPUs)(INT32 **handles, INT32* count);
typedef NvAPI_Status(*TYPE_NvAPI_GetMemoryInfo)(HANDLE hPhysicalGpu, NV_MEMORY_INFO* memInfo);
typedef NvAPI_Status(*TYPE_NvAPI_GPU_GetUsages)(HANDLE handle, NV_USAGES* Usages);
typedef NvAPI_Status(*TYPE_NvAPI_GPU_GetAllClocks)(HANDLE GpuHandle, NV_CLOCKS* Clocks);
typedef NvAPI_Status(*TYPE_NvAPI_GPU_GetAllClockFrequencies)(HANDLE GpuHandle, NV_GPU_CLOCK_FREQUENCIES_V2* Clocks);
typedef NvAPI_Status(*TYPE_NvAPI_GPU_GetPstatesInfo)(HANDLE GpuHandle, NV_GPU_PERF_PSTATES_INFO *PerfPstatesInfo);
typedef NvAPI_Status(*TYPE_NvAPI_GPU_GetPstates20)(HANDLE GpuHandle, NV_GPU_PERF_PSTATES20_INFO_V1 *pstates_info);
typedef NvAPI_Status(*TYPE_NvAPI_GPU_GetThermalSettings)(HANDLE PhysicalGpu, UINT32 SensorIndex, NV_GPU_THERMAL_SETTINGS* ThermalSettings);
typedef NvAPI_Status(*TYPE_NvAPI_GPU_GetVoltages)(HANDLE PhysicalGPU, NV_VOLTAGES* pPerfVoltages);
typedef NvAPI_Status(*TYPE_NvAPI_GPU_GetTachReading)(HANDLE PhysicalGPU, UINT32* pValue);
typedef NvAPI_Status(*TYPE_NvAPI_GPU_GetCoolerSettings)(HANDLE PhysicalGPU, UINT32 CoolerIndex, NV_GPU_COOLER_SETTINGS_V1* CoolerInfo);
typedef NvAPI_Status(*TYPE_NvAPI_GPU_SetCoolerLevels)(HANDLE PhysicalGPU, UINT32 coolerIndex, NV_GPU_SETCOOLER_LEVEL *pCoolerLevels);


BOOLEAN Nvapi_Initialize();
UINT8 Nvapi_GetLoad();
UINT16 Nvapi_GetEngineClock();
UINT16 Nvapi_GetMemoryClock();
UINT16 Nvapi_GetBaseEngineClock();
UINT16 Nvapi_GetBaseMemoryClock();
UINT64 Nvapi_GetTotalMemory();
UINT64 Nvapi_GetFreeMemory();
UINT8  Nvapi_GetTemperature();
UINT32 Nvapi_GetVoltage();
UINT32 Nvapi_GetFanSpeed();
UINT32 Nvapi_GetFanDutyCycle();
VOID Nvapi_ForceMaximumClocks();
VOID Nvapi_SetFanDutyCycle(int DutyCycle);
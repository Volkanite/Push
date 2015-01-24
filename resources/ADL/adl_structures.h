typedef struct ADLPMActivity
{
/// Must be set to the size of the structure
    int iSize;
/// Current engine clock.
    int EngineClock;
/// Current memory clock.
    int MemoryClock;
/// Current core voltage.
    int iVddc;
/// GPU utilization.
    int ActivityPercent;
/// Performance level index.
    int iCurrentPerformanceLevel;
/// Current PCIE bus speed.
    int iCurrentBusSpeed;
/// Number of PCIE bus lanes.
    int iCurrentBusLanes;
/// Maximum number of PCIE bus lanes.
    int iMaximumBusLanes;
/// Reserved for future purposes.
    int iReserved;
} ADLPMActivity;


typedef struct ADLTemperature
{
/// Must be set to the size of the structure
  int iSize;
/// Temperature in millidegrees Celsius.
  int Temperature;
} ADLTemperature;


typedef struct ADLODParameterRange
{
/// Minimum parameter value.
  int iMin;
/// Maximum parameter value.
  int Max;
/// Parameter step value.
  int iStep;
} ADLODParameterRange;


typedef struct ADLODParameters
{
/// Must be set to the size of the structure
  int Size;
/// Number of standard performance states.
  int NumberOfPerformanceLevels;
/// Indicates whether the GPU is capable to measure its activity.
  int iActivityReportingSupported;
/// Indicates whether the GPU supports discrete performance levels or performance range.
  int iDiscretePerformanceLevels;
/// Reserved for future use.
  int iReserved;
/// Engine clock range.
  ADLODParameterRange EngineClock;
/// Memory clock range.
  ADLODParameterRange MemoryClock;
/// Core voltage range.
  ADLODParameterRange sVddc;
} ADLODParameters;


typedef struct ADLODPerformanceLevel
{
/// Engine clock.
  int EngineClock;
/// Memory clock.
  int MemoryClock;
/// Core voltage.
  int Vddc;
} ADLODPerformanceLevel;


typedef struct ADLODPerformanceLevels
{
/// Must be set to sizeof( \ref ADLODPerformanceLevels ) + sizeof( \ref ADLODPerformanceLevel ) * (ADLODParameters.iNumberOfPerformanceLevels - 1)
  int Size;
  int iReserved;
/// Array of performance state descriptors. Must have ADLODParameters.iNumberOfPerformanceLevels elements.
  ADLODPerformanceLevel Levels [1];
} ADLODPerformanceLevels;

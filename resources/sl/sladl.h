class SlAdl{
    VOID* HeapHandle;
    UINT16 EngineClockMaximum;
    UINT16 MemoryClockMaximum;
    FLOAT VoltageMaximum;
    UINT8 PerformanceLevels;
public:
    SlAdl();
    UINT8 GetActivity();
    UINT16 GetEngineClock();
    UINT16 GetMemoryClock();
    UINT16 GetEngineClockMax();
    UINT16 GetMemoryClockMax();
    UINT8 GetTemperature();
    VOID SetEngineClock( UINT16 EngineClock );
    VOID SetMemoryClock( UINT16 MemoryClock );
    VOID SetVoltage( FLOAT Voltage );
    VOID SetMaxClocks();
};


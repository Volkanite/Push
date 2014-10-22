class CGPU{
public:
	virtual UINT16 GetEngineClock() = 0;
	virtual UINT16 GetMemoryClock() = 0;
	virtual UINT64 GetTotalMemory() = 0;
	virtual UINT64 GetFreeMemory() = 0;
	virtual UINT8 GetTemperature() = 0;
	virtual UINT8 GetLoad() = 0;
	virtual VOID ForceMaximumClocks() = 0;
};
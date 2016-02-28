class ThreadMonitor{
    UINT64 MaxThreadCyclesDelta;
public:
    ThreadMonitor();
    VOID Refresh();
    UINT8 GetMaxThreadUsage();
};

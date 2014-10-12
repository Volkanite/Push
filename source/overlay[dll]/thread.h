class ThreadMonitor{
    //UINT8 MaxThreadUsage;
    UINT64 MaxThreadCyclesDelta;
public:
    ThreadMonitor();
    VOID Refresh();
    UINT8 GetMaxThreadUsage();
    VOID OptimizeThreads();
};

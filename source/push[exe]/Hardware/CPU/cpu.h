VOID CPU_Intialize();
VOID CPU_ReadMsr(DWORD Index, DWORD* EAX, DWORD* EDX);

/* stats */
UINT8 CPU_GetTemperature();
UINT16 CPU_GetSpeed();
UINT16 CPU_GetNormalSpeed();
UINT16 CPU_GetMaxSpeed();
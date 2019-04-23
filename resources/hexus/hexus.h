typedef struct DETOUR_PROPERTIES
{
    LPVOID Origin;
    LPVOID Trampoline;
    size_t Length;
}DETOUR_PROPERTIES;


#ifdef __cplusplus
extern "C" {
#endif

void* DetourCreate(LPVOID lpFuncOrig, LPVOID lpFuncDetour, DETOUR_PROPERTIES *Properties);
BOOL DetourDestroy(DETOUR_PROPERTIES* Properties);

#ifdef __cplusplus
}
#endif

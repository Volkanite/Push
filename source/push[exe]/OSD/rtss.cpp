#include <sl.h>
#include <string.h>
#include <RTSSSharedMemory.h>
#include <push.h>


LPRTSS_SHARED_MEMORY RTSSSharedMemory;


extern "C" HANDLE __stdcall OpenFileMappingW(
    _In_ DWORD   dwDesiredAccess,
    _In_ INTBOOL    bInheritHandle,
    _In_ WCHAR* lpName
    );

extern "C" VOID* __stdcall MapViewOfFile(
    _In_ HANDLE hFileMappingObject,
    _In_ DWORD  dwDesiredAccess,
    _In_ DWORD  dwFileOffsetHigh,
    _In_ DWORD  dwFileOffsetLow,
    _In_ SIZE_T dwNumberOfBytesToMap
    );


VOID Initialize()
{
    HANDLE MapFile;
    MapFile = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, L"RTSSSharedMemoryV2");
    RTSSSharedMemory = (LPRTSS_SHARED_MEMORY) MapViewOfFile(MapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
}


void RTSS_Update( OSD_ITEM* OsdItems )
{
    UINT8 i;
    OSD_ITEM *osdItem;
    osdItem = OsdItems;
    WCHAR osdText[256];
    CHAR ansiText[256];
    BOOLEAN stuffToDraw = FALSE;

    for (i = 0; i < PushSharedMemory->NumberOfOsdItems; i++, osdItem++)
    {
        if (!osdItem->Flag //draw if no flag, could be somebody just wants to display stuff on-screen
            || PushSharedMemory->OSDFlags & osdItem->Flag //if it has a flag, is it set?
            || (osdItem->Threshold && osdItem->Value > osdItem->Threshold)) //is the item's value > it's threshold?
        {
            if (i == 0)
                String::Copy(osdText, osdItem->Text);
            else
                String::Concatenate(osdText, osdItem->Text);

            stuffToDraw = TRUE;
        }
    }

    if (!stuffToDraw) return;
    if (!RTSSSharedMemory) Initialize();
    if (!RTSSSharedMemory) return;

    WideCharToMultiByte(CP_ACP, 0, osdText, -1, ansiText, 256, NULL, NULL);

    for (DWORD dwPass = 0; dwPass<2; dwPass++)
        //1st pass : find previously captured OSD slot
        //2nd pass : otherwise find the first unused OSD slot and capture it
    {
        for (DWORD dwEntry = 0; dwEntry<RTSSSharedMemory->dwOSDArrSize; dwEntry++)
        {
            RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_OSD_ENTRY pEntry = (RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_OSD_ENTRY)((BYTE*)RTSSSharedMemory + RTSSSharedMemory->dwOSDArrOffset + dwEntry * RTSSSharedMemory->dwOSDEntrySize);

            if (dwPass)
            {
                if (!strlen(pEntry->szOSDOwner)) strcpy(pEntry->szOSDOwner, "RTSSPush");
            }

            if (!strcmp(pEntry->szOSDOwner, "RTSSPush"))
            {
                strncpy(pEntry->szOSD, ansiText, sizeof(pEntry->szOSD) - 1);

                RTSSSharedMemory->dwOSDFrame++;

                break;
            }
        }

    }
}